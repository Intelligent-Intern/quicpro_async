/*
 * src/websocket.c – C Implementation for WebSocket over QUIC/HTTP/3
 * ===================================================================
 *
 * This file implements the C-native logic for the WebSocket API. It handles
 * upgrading an HTTP/3 stream to a WebSocket connection via the CONNECT method,
 * as well as the framing protocol for sending and receiving WebSocket messages
 * over the underlying QUIC stream.
 */

#include "php_quicpro.h"
#include "websocket.h"
#include "session.h"
#include "cancel.h"
#include "mcp.h" /* For quicpro_mcp_connect if used by ws_connect */

#include <quiche.h>
#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_smart_str.h>
#include <zend_string.h>

/* --- WebSocket Constants --- */
#define WS_OPCODE_CONTINUATION 0x0
#define WS_OPCODE_TEXT         0x1
#define WS_OPCODE_BINARY       0x2
#define WS_OPCODE_CLOSE        0x8
#define WS_OPCODE_PING         0x9
#define WS_OPCODE_PONG         0xA

/* --- Internal WebSocket Connection State --- */
typedef enum {
    WS_STATE_CONNECTING,
    WS_STATE_OPEN,
    WS_STATE_CLOSING,
    WS_STATE_CLOSED
} quicpro_ws_state_t;

/*
 * C struct to represent an active WebSocket connection.
 * This will be managed as a PHP resource of type `le_quicpro_ws`.
 */
typedef struct _quicpro_ws_conn_internal {
    quicpro_session_t *session; /* Pointer to the underlying QUIC/H3 session */
    uint64_t stream_id;         /* The QUIC stream ID used for this WebSocket connection */
    quicpro_ws_state_t state;   /* The current state of the WebSocket connection */
    zend_string *last_error;    /* Last error message specific to this connection */
    /* Buffers for handling fragmented frames would be added here */
    smart_str read_buffer;
} quicpro_ws_conn_internal_t;


/* --- Resource Destructor --- */
/* This function is called by the Zend Engine when a WebSocket resource is garbage collected. */
static void quicpro_ws_conn_dtor(zend_resource *rsrc) {
    quicpro_ws_conn_internal_t *ws_conn = (quicpro_ws_conn_internal_t *)rsrc->ptr;
    if (ws_conn) {
        /*
         * Note: The underlying quicpro_session_t is managed as its own resource
         * and should NOT be freed here. This destructor only handles resources
         * owned directly by the WebSocket connection wrapper itself.
         */
        if (ws_conn->last_error) {
            zend_string_release(ws_conn->last_error);
        }
        smart_str_free(&ws_conn->read_buffer);
        efree(ws_conn);
    }
}

/* --- Static Helper Functions --- */

/*
 * Creates a WebSocket frame header for a client-to-server message.
 * A full implementation would handle masking, larger payload lengths, etc.
 * This is a simplified version for unfragmented text/binary messages.
 */
static void create_ws_frame_header(smart_str *buf, uint8_t opcode, size_t payload_len, zend_bool is_masked) {
    uint8_t header[14];
    size_t header_len = 0;

    /* First byte: FIN bit set (for unfragmented message) and opcode */
    header[0] = 0x80 | (opcode & 0x0F);

    /* Second byte: Mask bit and payload length */
    header[1] = is_masked ? 0x80 : 0x00;
    if (payload_len <= 125) {
        header[1] |= (uint8_t)payload_len;
        header_len = 2;
    } else if (payload_len <= 65535) {
        header[1] |= 126;
        header[2] = (uint8_t)(payload_len >> 8);
        header[3] = (uint8_t)(payload_len & 0xFF);
        header_len = 4;
    } else {
        header[1] |= 127;
        uint64_t len64 = payload_len; // Use 64-bit var for shifting
        header[2] = (uint8_t)(len64 >> 56);
        header[3] = (uint8_t)(len64 >> 48);
        header[4] = (uint8_t)(len64 >> 40);
        header[5] = (uint8_t)(len64 >> 32);
        header[6] = (uint8_t)(len64 >> 24);
        header[7] = (uint8_t)(len64 >> 16);
        header[8] = (uint8_t)(len64 >> 8);
        header[9] = (uint8_t)(len64 & 0xFF);
        header_len = 10;
    }

    smart_str_appendl(buf, (char*)header, header_len);
}

/*
 * Parses a WebSocket frame header from the read buffer.
 * Returns the opcode and payload length. A full implementation would also return FIN, RSV bits, and masking key.
 * This is a simplified version.
 */
static zend_bool parse_ws_frame_header(smart_str *read_buffer, uint8_t *opcode_out, uint64_t *payload_len_out, size_t *header_len_out) {
    if (read_buffer->s == NULL || ZSTR_LEN(read_buffer->s) < 2) return 0; // Not enough data for a minimal header

    const unsigned char *p = (const unsigned char*)ZSTR_VAL(read_buffer->s);
    size_t buffer_len = ZSTR_LEN(read_buffer->s);
    zend_bool fin = (p[0] & 0x80);
    *opcode_out = (p[0] & 0x0F);

    zend_bool is_masked = (p[1] & 0x80);
    uint64_t len = (p[1] & 0x7F);
    size_t current_pos = 2;

    if (len == 126) {
        if (buffer_len < 4) return 0;
        *payload_len_out = ((uint64_t)p[2] << 8) | p[3];
        current_pos = 4;
    } else if (len == 127) {
        if (buffer_len < 10) return 0;
        /* Note: zend_long might truncate on 32-bit PHP. Use uint64_t. */
        *payload_len_out = ((uint64_t)p[2] << 56) | ((uint64_t)p[3] << 48) | ((uint64_t)p[4] << 40) |
                           ((uint64_t)p[5] << 32) | ((uint64_t)p[6] << 24) | ((uint64_t)p[7] << 16) |
                           ((uint64_t)p[8] << 8)  | p[9];
        current_pos = 10;
    } else {
        *payload_len_out = len;
    }

    if (is_masked) {
        if (buffer_len < current_pos + 4) return 0;
        /* TODO: A real client needs to store and use this mask to unmask the payload. */
        current_pos += 4;
    }

    *header_len_out = current_pos;
    return 1;
}

/* --- PHP_FUNCTION Implementations --- */

PHP_FUNCTION(quicpro_ws_upgrade)
{
    zval *z_session_res;
    char *path;
    size_t path_len;
    zval *headers_php_array = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(z_session_res)
        Z_PARAM_STRING(path, path_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(headers_php_array)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *session = (quicpro_session_t *)zend_fetch_resource_ex(z_session_res, "Quicpro MCP Session", le_quicpro_session);
    if (!session || !session->conn || !session->h3) {
        throw_mcp_error_as_php_exception(0, "Invalid or closed connection resource provided for WebSocket upgrade.");
        RETURN_FALSE;
    }

    /*
     * Implementation Note: This function needs to:
     * 1. Construct H3 headers for a CONNECT request, including `:method: CONNECT`, `:protocol: websocket`, etc.
     * 2. Send the request using `quiche_h3_send_request`.
     * 3. Enter a polling loop (like `mcp_poll_for_response` from mcp.c) to wait for a 2xx response.
     * 4. If a 2xx response is received, the stream is considered upgraded.
     * 5. Create a `quicpro_ws_conn_internal_t` struct, populate it with the session pointer and stream ID.
     * 6. Register this struct as a new PHP resource of type `le_quicpro_ws`.
     * 7. Return the new WebSocket resource.
     * This is a complex, synchronous operation.
     */
    zend_error(E_WARNING, "quicpro_ws_upgrade() is not yet fully implemented.");
    RETURN_FALSE;
}


PHP_FUNCTION(quicpro_ws_connect)
{
    /* This function would be a convenience wrapper around quicpro_mcp_connect() followed by quicpro_ws_upgrade(). */
    zend_error(E_WARNING, "quicpro_ws_connect() is not yet fully implemented.");
    RETURN_FALSE;
}


PHP_FUNCTION(quicpro_ws_send)
{
    zval *z_ws_conn_res;
    char *data;
    size_t data_len;
    zend_bool is_binary = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(z_ws_conn_res)
        Z_PARAM_STRING(data, data_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(is_binary)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_ws_conn_internal_t *ws_conn = (quicpro_ws_conn_internal_t *)zend_fetch_resource_ex(z_ws_conn_res, "Quicpro WebSocket Connection", le_quicpro_ws);
    if (!ws_conn || ws_conn->state != WS_STATE_OPEN) {
        /* Throw or set error: connection not open */
        RETURN_FALSE;
    }

    smart_str frame_buf = {0};
    uint8_t opcode = is_binary ? WS_OPCODE_BINARY : WS_OPCODE_TEXT;

    /* Create the WebSocket frame header. Client-to-server frames must be masked. */
    create_ws_frame_header(&frame_buf, opcode, data_len, 1 /* is_masked */);

    /* Generate a 4-byte masking key */
    uint32_t masking_key_int;
    if (RAND_bytes((unsigned char*)&masking_key_int, 4) != 1) {
        /* OpenSSL RAND_bytes error */
        smart_str_free(&frame_buf);
        throw_mcp_error_as_php_exception(0, "Failed to generate WebSocket masking key.");
        RETURN_FALSE;
    }
    smart_str_appendl(&frame_buf, (char*)&masking_key_int, 4);

    /* Append the masked payload */
    size_t frame_buf_start_len = ZSTR_LEN(frame_buf.s);
    smart_str_appendl(&frame_buf, data, data_len);
    unsigned char* payload_ptr = (unsigned char*)ZSTR_VAL(frame_buf.s) + frame_buf_start_len;
    unsigned char* mask_ptr = (unsigned char*)&masking_key_int;
    for (size_t i = 0; i < data_len; ++i) {
        payload_ptr[i] ^= mask_ptr[i % 4];
    }

    /* Send the complete frame over the QUIC stream */
    ssize_t sent = quiche_stream_send(ws_conn->session->conn, ws_conn->stream_id, (uint8_t*)ZSTR_VAL(frame_buf.s), ZSTR_LEN(frame_buf.s), 1 /* fin=false, stream stays open */);
    smart_str_free(&frame_buf);

    if (sent < 0) {
        /* Note: quiche_stream_send is for raw QUIC streams. If MCP is on H3, we'd still use quiche_h3_send_body.
         * This assumes the stream has been "unwrapped" from H3 after the CONNECT upgrade.
         */
        throw_quiche_error_as_php_exception((int)sent, "WebSocket send failed on QUIC stream.");
        RETURN_FALSE;
    }

    RETURN_TRUE;
}


PHP_FUNCTION(quicpro_ws_receive)
{
    /*
     * TODO: Implementation for receiving WebSocket messages.
     * 1. Parse parameters: WebSocket connection resource, optional timeout.
     * 2. Enter a polling loop (using `select()` or `epoll()` on the socket).
     * 3. Within the loop, call `quiche_stream_recv()` to read raw bytes from the stream into `ws_conn->read_buffer`.
     * 4. Call `parse_ws_frame_header()` on the `read_buffer`.
     * 5. If a full frame header is present, determine the total frame length (header + payload).
     * 6. If the full frame is in the buffer, process it:
     * - Handle control frames (Ping -> send Pong, Close -> handle shutdown, Pong -> ignore).
     * - For data frames (Text, Binary, Continuation), unmask the payload (server-to-client frames are not masked).
     * - Append payload to a message buffer (to handle fragmentation).
     * - If FIN bit was set, the message is complete. Return the message payload as a PHP string.
     * - Remove the processed frame from `ws_conn->read_buffer`.
     * 7. Loop until a full message is decoded or timeout occurs.
     */
    zend_error(E_WARNING, "quicpro_ws_receive() is not yet fully implemented.");
    RETURN_FALSE;
}


PHP_FUNCTION(quicpro_ws_close)
{
    /*
     * TODO: Implementation for graceful WebSocket close.
     * 1. Fetch WebSocket connection resource.
     * 2. If state is OPEN, create and send a WebSocket Close frame.
     * 3. Enter a polling loop to wait for the peer's responding Close frame.
     * 4. Once peer's Close frame is received, or after a timeout, call `quiche_conn_stream_shutdown`
     * on the underlying QUIC stream.
     * 5. Update ws_conn->state to CLOSED and free the resource.
     */
    zend_error(E_WARNING, "quicpro_ws_close() is not yet fully implemented.");
    RETURN_FALSE;
}


PHP_FUNCTION(quicpro_ws_get_status)
{
    zval *z_ws_conn_res;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_ws_conn_res)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_ws_conn_internal_t *ws_conn = (quicpro_ws_conn_internal_t *)zend_fetch_resource_ex(z_ws_conn_res, "Quicpro WebSocket Connection", le_quicpro_ws);
    if (!ws_conn) {
        RETURN_FALSE;
    }
    RETURN_LONG(ws_conn->state);
}


PHP_FUNCTION(quicpro_ws_get_last_error)
{
    /* TODO: Return a global or per-connection WebSocket error string */
    RETURN_STRING("WebSocket error reporting not yet fully implemented.");
}