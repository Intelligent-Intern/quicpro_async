/**
 * @file extension/src/server/early_hints.c
 * @brief Implementation for sending HTTP 103 Early Hints responses.
 *
 * This file contains the C-level logic to construct and send one or more
 * 103 Early Hints responses on a given stream, allowing for client-side
 * preloading of resources.
 */

#include <php.h>
#include <zend_exceptions.h>
#include <zend_hash.h>

#include "server/early_hints.h"
#include "session/session.h" // For quicpro_session_t and stream management
#include "quiche.h"

extern int le_quicpro_session;

PHP_FUNCTION(quicpro_server_send_early_hints)
{
    zval *session_resource;
    zend_long stream_id;
    zval *hints_array;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(session_resource)
        Z_PARAM_LONG(stream_id)
        Z_PARAM_ARRAY(hints_array)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *session = (quicpro_session_t *)zend_fetch_resource(Z_RES_P(session_resource), "quicpro_session", le_quicpro_session);
    if (!session || !session->conn) {
        zend_throw_exception(NULL, "Invalid session resource provided.", 0);
        RETURN_FALSE;
    }

    if (quiche_conn_is_closed(session->conn)) {
        RETURN_FALSE;
    }

    // Find the stream and check its state.
    quicpro_stream_t *stream = zend_hash_index_find_ptr(session->streams, (zend_ulong)stream_id);
    if (!stream) {
        zend_throw_exception_ex(NULL, 0, "Stream with ID %ld not found in the current session.", stream_id);
        RETURN_FALSE;
    }

    if (stream->final_response_sent) {
        zend_throw_exception_ex(NULL, 0, "Cannot send early hints on stream %ld after a final response has already been sent.", stream_id);
        RETURN_FALSE;
    }

    HashTable *ht = Z_ARRVAL_P(hints_array);
    zval *entry;
    // We need one extra header for the ":status" pseudo-header.
    int header_count = zend_hash_num_elements(ht) + 1;

    if (header_count <= 1) {
        RETURN_TRUE; // Nothing to send.
    }

    quiche_h3_header *headers = safe_emalloc(header_count, sizeof(quiche_h3_header), 0);

    // Manually add the 103 status pseudo-header first.
    headers[0].name = (const uint8_t *)":status";
    headers[0].name_len = sizeof(":status") - 1;
    headers[0].value = (const uint8_t *)"103";
    headers[0].value_len = sizeof("103") - 1;

    int i = 1; // Start populating from the second element.

    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (Z_TYPE_P(entry) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(entry)) != 2) {
            continue; // Each entry must be a [key, value] pair.
        }

        zval *header_name_zv = zend_hash_index_find(Z_ARRVAL_P(entry), 0);
        zval *header_value_zv = zend_hash_index_find(Z_ARRVAL_P(entry), 1);

        if (header_name_zv && header_value_zv &&
            Z_TYPE_P(header_name_zv) == IS_STRING && Z_TYPE_P(header_value_zv) == IS_STRING)
        {
            headers[i].name = (const uint8_t *)Z_STRVAL_P(header_name_zv);
            headers[i].name_len = Z_STRLEN_P(header_name_zv);
            headers[i].value = (const uint8_t *)Z_STRVAL_P(header_value_zv);
            headers[i].value_len = Z_STRLEN_P(header_value_zv);
            i++;
        }
    } ZEND_HASH_FOREACH_END();

    // Send the informational response. The `fin` flag must be false.
    ssize_t written = quiche_h3_send_response(
        session->conn,
        stream_id,
        headers,
        i, // The actual number of valid headers found (including status)
        false // `false` for `fin` indicates this is not the final response.
    );

    efree(headers);

    if (written < 0) {
        zend_throw_exception_ex(NULL, 0, "Failed to send early hints, quiche error: %ld", written);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
