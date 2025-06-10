/*
 * src/mcp.c – C Implementation for Model Context Protocol (MCP) Client Operations
 * ==============================================================================
 *
 * This file implements the C-native logic for the MCP client. It provides
 * functions to connect to MCP agents over QUIC/H3, send synchronous requests,
 * and handle streaming data. This layer acts as a foundational component for

 * the PipelineOrchestrator and for any direct userland MCP interactions.
 */

#include "php_quicpro.h"
#include "mcp.h"
#include "config.h"             /* For quicpro_cfg_t and qp_fetch_cfg */
#include "session.h"            /* For quicpro_session_t */
#include "proto_internal.h"     /* For direct access to compiled schema structs */
#include "cancel.h"             /* For error throwing helpers */
#include "http3.h"              /* For reusing H3 logic if MCP is on H3 */

#include <quiche.h>
#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_smart_str.h>
#include <zend_string.h>

/* --- Static Helper Function Prototypes --- */
/* A helper to run a polling loop for a specific stream until a response is complete or timeout occurs. */
static int mcp_poll_for_response(quicpro_session_t *session, uint64_t stream_id, smart_str *response_body_buf, zend_long timeout_ms);


/* --- PHP_FUNCTION Implementations --- */

/* Corresponds to `new Quicpro\MCP($host, $port, $configResource, $options)` */
PHP_FUNCTION(quicpro_mcp_connect)
{
    char *host;
    size_t host_len;
    zend_long port;
    zval *z_cfg;
    zval *options = NULL;

    /* A configuration resource is required to define QUIC/TLS parameters. */
    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(z_cfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(options)
    ZEND_PARSE_PARAMETERS_END();

    /*
     * NOTE: This function's logic would be very similar to `quicpro_connect` from connect.c.
     * It performs DNS lookup, socket creation, and initiates a QUIC/H3 connection.
     * It uses a quicpro_cfg_t resource for configuration.
     * For brevity, we assume a C helper `quicpro_session_open_c()` exists (conceptually
     * from session.c) that encapsulates this logic and returns a quicpro_session_t*.
     */

    quicpro_cfg_t *config_wrapper = qp_fetch_cfg(z_cfg);
    if (!config_wrapper || !config_wrapper->cfg) {
        throw_mcp_error_as_php_exception(0, "Invalid configuration resource provided to quicpro_mcp_connect.");
        RETURN_FALSE;
    }
    if (config_wrapper->frozen) {
        /* Optionally allow reusing frozen configs, but warn or note it. */
    }

    /* Concept: quicpro_session_open_c would perform DNS, socket, connect, quiche_connect, and h3 init */
    /* It would return a fully initialized session struct or NULL on failure. */
    /* quicpro_session_t *session = quicpro_session_open_c(host, port, config_wrapper->cfg, options); */

    /* For this skeleton, we'll placeholder the session creation. */
    /* In a real implementation, you would reuse the logic from your `connect.c`. */
    quicpro_session_t *session = ecalloc(1, sizeof(quicpro_session_t));
    session->host = estrndup(host, host_len);
    /* ... imagine full quiche_connect and quiche_h3_conn_new_with_transport happening here ... */
    /* For now, this is just a placeholder allocation. */
    session->conn = (void*)0xDEADBEEF; // Placeholder to signify it's "connected"
    session->h3 = (void*)0xCAFEBABE;   // Placeholder

    if (!session) {
        /* quicpro_session_open_c would have thrown the specific error */
        RETURN_FALSE;
    }

    /* Mark the config as used and immutable. */
    quicpro_cfg_mark_frozen(z_cfg);

    /* Register the session struct as a new resource and return it. */
    RETURN_RES(zend_register_resource(session, le_quicpro_session));
}

PHP_FUNCTION(quicpro_mcp_close)
{
    zval *z_session_res;
    quicpro_session_t *session;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_session_res)
    ZEND_PARSE_PARAMETERS_END();

    session = (quicpro_session_t *)zend_fetch_resource_ex(z_session_res, "Quicpro MCP Session", le_quicpro_session);
    if (!session) {
        RETURN_FALSE;
    }

    if (session->conn) {
        /* Gracefully close the QUIC connection with an application-defined code and reason. */
        quiche_conn_close(session->conn, true, 0x00, (const uint8_t *)"MCP_CLOSE", sizeof("MCP_CLOSE")-1);
        /* The actual freeing of the session struct and socket closure is handled
         * by the resource destructor (quicpro_session_dtor). */
    }

    /* It's good practice to remove the resource from the list to prevent further use,
     * which effectively invalidates it immediately. */
    zend_list_close(Z_RES_P(z_session_res));

    RETURN_TRUE;
}


PHP_FUNCTION(quicpro_mcp_request)
{
    zval *z_session_res;
    char *service_name, *method_name, *request_payload_binary;
    size_t service_name_len, method_name_len, request_payload_len;
    zval *per_request_options = NULL;
    uint64_t stream_id;

    ZEND_PARSE_PARAMETERS_START(4, 5)
        Z_PARAM_RESOURCE(z_session_res)
        Z_PARAM_STRING(service_name, service_name_len)
        Z_PARAM_STRING(method_name, method_name_len)
        Z_PARAM_STRING(request_payload_binary, request_payload_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(per_request_options)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *session = (quicpro_session_t *)zend_fetch_resource_ex(z_session_res, "Quicpro MCP Session", le_quicpro_session);
    if (!session || !session->conn || !session->h3) {
        throw_mcp_error_as_php_exception(0, "Invalid or closed MCP connection resource provided.");
        RETURN_FALSE;
    }

    /*
     * This is a simplified reimplementation of `quicpro_send_request` logic from http3.c,
     * tailored for MCP RPC-style calls.
     */
    quiche_h3_header headers[5];
    char path[256];
    int path_len = snprintf(path, sizeof(path), "/%s/%s", service_name, method_name);

    headers[0] = (quiche_h3_header){ .name = (uint8_t *)":method", .name_len = 7, .value = (uint8_t *)"POST", .value_len = 4 };
    headers[1] = (quiche_h3_header){ .name = (uint8_t *)":scheme", .name_len = 7, .value = (uint8_t *)"https", .value_len = 5 };
    headers[2] = (quiche_h3_header){ .name = (uint8_t *)":path", .name_len = 5, .value = (uint8_t *)path, .value_len = path_len };
    headers[3] = (quiche_h3_header){ .name = (uint8_t *)":authority", .name_len = 10, .value = (uint8_t *)session->host, .value_len = strlen(session->host) };
    headers[4] = (quiche_h3_header){ .name = (uint8_t *)"content-type", .name_len = 12, .value = (uint8_t *)"application/vnd.quicpro.proto", .value_len = 29 };

    stream_id = quiche_h3_send_request(session->h3, session->conn, headers, 5, 0);
    if ((int64_t)stream_id < 0) {
        throw_quiche_error_as_php_exception((int)stream_id, "MCP request failed: could not send H3 headers for service '%s'", service_name);
        RETURN_FALSE;
    }

    ssize_t sent = quiche_h3_send_body(session->h3, session->conn, stream_id, (uint8_t *)request_payload_binary, request_payload_len, 1 /* final chunk */);
    if (sent < 0) {
        throw_quiche_error_as_php_exception((int)sent, "MCP request failed: could not send H3 body for service '%s'", service_name);
        RETURN_FALSE;
    }

    /*
     * Now, because this is a synchronous request-response function, we must
     * block and poll for the full response on this stream_id.
     */
    smart_str response_body_buf = {0};
    zend_long timeout_ms = 30000; // Default timeout, could be overridden from per_request_options
    if (per_request_options && Z_TYPE_P(per_request_options) == IS_ARRAY) {
        zval *zv_timeout = zend_hash_str_find(Z_ARRVAL_P(per_request_options), "timeout_ms", sizeof("timeout_ms")-1);
        if (zv_timeout && Z_TYPE_P(zv_timeout) == IS_LONG) {
            timeout_ms = Z_LVAL_P(zv_timeout);
        }
    }

    if (mcp_poll_for_response(session, stream_id, &response_body_buf, timeout_ms) == FAILURE) {
        /* Error already thrown inside polling function */
        smart_str_free(&response_body_buf);
        RETURN_FALSE;
    }

    if (response_body_buf.s) {
        RETVAL_STR(response_body_buf.s); /* Gives ownership of the zend_string */
    } else {
        RETVAL_EMPTY_STRING();
    }
}

PHP_FUNCTION(quicpro_mcp_upload_from_stream)
{
    /*
     * TODO: Implementation for client-streaming RPC.
     * 1. Parse parameters: connection resource, service/method names, stream_identifier, PHP readable stream resource.
     * 2. Send initial H3 request with headers (and optional initial_metadata_payload). The 'fin' flag on send_body would be 0.
     * 3. Enter a loop:
     * - Read a chunk from the PHP stream resource using `php_stream_read()`.
     * - Send the chunk as an H3 DATA frame using `quiche_h3_send_body()`. The 'fin' flag is 1 only for the last chunk.
     * - Must handle backpressure from quiche (if send_body returns QUICHE_ERR_BUFFER_TOO_SMALL, need to call quicpro_poll and retry).
     * 4. After the stream is fully sent, enter a polling loop (like `mcp_poll_for_response`) to wait for a single summary response from the server.
     * 5. Return true/false based on success.
     */
    zend_error(E_WARNING, "quicpro_mcp_upload_from_stream() is not yet implemented.");
    RETURN_FALSE;
}

PHP_FUNCTION(quicpro_mcp_download_to_stream)
{
    /*
     * TODO: Implementation for server-streaming RPC.
     * 1. Parse parameters: connection resource, service/method names, request payload, PHP writable stream resource.
     * 2. Send the H3 request with the request_payload_binary to initiate the server stream.
     * 3. Enter a polling loop:
     * - Call `quicpro_poll` or similar logic.
     * - Check for H3 DATA events for the request's stream_id.
     * - As data chunks are received via `quiche_h3_recv_body()`, write them to the PHP writable stream using `php_stream_write()`.
     * - Loop until a FIN flag is received on the stream from the server.
     * 4. Return true/false based on success.
     */
    zend_error(E_WARNING, "quicpro_mcp_download_to_stream() is not yet implemented.");
    RETURN_FALSE;
}

PHP_FUNCTION(quicpro_mcp_get_error)
{
    /* TODO: Return last error message for the MCP module.
     * This could be a global string (with TSRM for ZTS) or fetched from a session struct.
     * For now, it could be a placeholder.
     */
    RETURN_STRING("MCP error reporting is not yet fully implemented.");
}


/* --- C Helper Implementation for Synchronous Polling --- */

static int mcp_poll_for_response(quicpro_session_t *session, uint64_t stream_id, smart_str *response_body_buf, zend_long timeout_ms) {
    /*
     * This is a simplified polling loop. A real implementation needs to be
     * more robust, using select() or poll() on the socket for efficient waiting,
     * and properly handling quiche's own timeout recommendations.
     */
    zend_long start_time = (zend_long)(time(NULL) * 1000); // Rough start time in ms

    while (1) {
        /* Check for overall timeout */
        if (timeout_ms > 0 && ((zend_long)(time(NULL) * 1000) - start_time) > timeout_ms) {
            throw_mcp_error_as_php_exception(0, "MCP request timed out after %ld ms while waiting for response on stream %llu.", timeout_ms, (unsigned long long)stream_id);
            return FAILURE;
        }

        /*
         * This polling logic would be very similar to what's in `poll.c`,
         * but focused on finding the response for a specific stream.
         * For this skeleton, we'll simulate the core logic.
         */

        /* Step 1: Drain egress queue */
        /* You must always send what's in quiche's buffer */
        /* A real implementation would have a dedicated send_all_pending() helper */

        /* Step 2: Read from socket and feed to quiche */
        /* A real implementation would have a dedicated recv_and_process() helper */

        /* Step 3: Poll for H3 events */
        quiche_h3_event *ev;
        while (quiche_h3_conn_poll(session->h3, session->conn, &ev) > 0) {
            if (quiche_h3_event_stream_id(ev) != stream_id) {
                // Event for another stream. Ignore for this simple synchronous case.
                // A more advanced client would need to queue or handle this.
                continue;
            }

            switch (quiche_h3_event_type(ev)) {
                case QUICHE_H3_EVENT_HEADERS:
                    /* Response headers received. Check status code. */
                    /* A full implementation would parse headers and check for e.g. :status != 200 */
                    break;

                case QUICHE_H3_EVENT_DATA: {
                    /* Data received for our stream. */
                    uint8_t buf[8192];
                    ssize_t read_len = quiche_h3_recv_body(session->h3, session->conn, stream_id, buf, sizeof(buf));
                    if (read_len < 0) {
                         throw_quiche_error_as_php_exception((int)read_len, "Failed to receive MCP response body on stream %llu", (unsigned long long)stream_id);
                         return FAILURE;
                    }
                    smart_str_appendl(response_body_buf, (char*)buf, read_len);
                    break;
                }
                case QUICHE_H3_EVENT_FINISHED:
                    /* The stream is fully closed. We have our complete response. */
                    return SUCCESS;
                default:
                    break;
            }
        }

        /* Step 4: Check if connection died */
        if (quiche_conn_is_closed(session->conn)) {
            throw_mcp_error_as_php_exception(0, "MCP connection closed while waiting for response on stream %llu.", (unsigned long long)stream_id);
            return FAILURE;
        }

        /* Give the OS a small break to avoid pinning the CPU at 100% in this simple loop */
        usleep(1000); // 1ms sleep
    }

    return FAILURE; // Should not be reached unless timeout logic is flawed
}