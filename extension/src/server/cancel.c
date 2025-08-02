/**
 * @file extension/src/server/cancel.c
 * @brief Implementation for handling request cancellation.
 *
 * This file contains the C-level logic to manage cancellation handlers. It
 * provides the mechanism to associate a PHP callback with an active stream
 * and invoke it when the client signals it no longer needs the response
 * (e.g., by sending a QUIC RST_STREAM frame).
 */

#include <php.h>
#include <zend_exceptions.h>

#include "server/cancel.h"
#include "session/session.h" // For quicpro_session_t and quicpro_stream_t

// Resource IDs defined in the main extension file.
extern int le_quicpro_session;

/**
 * The actual invocation of the cancellation handler.
 * This internal function is called by the event loop when a stream reset is detected.
 */
void quicpro_internal_invoke_cancel_handler(quicpro_stream_t *stream)
{
    if (!stream || !stream->has_cancel_handler) {
        return;
    }

    zval retval;
    zval params[1];
    ZVAL_LONG(&params[0], stream->stream_id);

    stream->cancel_fci.param_count = 1;
    stream->cancel_fci.params = params;
    stream->cancel_fci.retval = &retval;

    // Call the stored PHP function.
    if (zend_call_function(&stream->cancel_fci, &stream->cancel_fcc) == SUCCESS) {
        zval_ptr_dtor(&retval);
    }

    // Clean up the stored callable to prevent double-invocation and memory leaks.
    if (Z_TYPE(stream->cancel_fci.function_name) != IS_UNDEF) {
        zval_ptr_dtor(&stream->cancel_fci.function_name);
        ZVAL_UNDEF(&stream->cancel_fci.function_name);
    }
    if (stream->cancel_fci.object) {
        OBJ_RELEASE(Z_OBJ(stream->cancel_fci.object));
        stream->cancel_fci.object = NULL;
    }
    stream->has_cancel_handler = false;
}


PHP_FUNCTION(quicpro_server_on_cancel)
{
    zval *session_resource;
    zend_long stream_id;
    zend_fcall_info fci = empty_fcall_info;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(session_resource)
        Z_PARAM_LONG(stream_id)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *session = (quicpro_session_t *)zend_fetch_resource(Z_RES_P(session_resource), "quicpro_session", le_quicpro_session);
    if (!session) {
        zend_throw_exception(NULL, "Invalid session resource provided.", 0);
        RETURN_FALSE;
    }

    // Find the specific stream object within the session's stream hash table.
    quicpro_stream_t *stream = zend_hash_index_find_ptr(session->streams, (zend_ulong)stream_id);
    if (!stream) {
        zend_throw_exception_ex(NULL, 0, "Stream with ID %ld not found in the current session.", stream_id);
        RETURN_FALSE;
    }

    // If a handler was previously set, clean it up first.
    if (stream->has_cancel_handler) {
        if (Z_TYPE(stream->cancel_fci.function_name) != IS_UNDEF) {
            zval_ptr_dtor(&stream->cancel_fci.function_name);
        }
        if (stream->cancel_fci.object) {
            OBJ_RELEASE(Z_OBJ(stream->cancel_fci.object));
        }
    }

    // Store the new callable information. We must increment the refcount of any
    // zvals inside to prevent them from being garbage-collected by PHP.
    stream->cancel_fci = fci;
    stream->cancel_fcc = fcc;
    stream->has_cancel_handler = true;

    Z_TRY_ADDREF(stream->cancel_fci.function_name);
    if (stream->cancel_fci.object) {
        Z_TRY_ADDREF(stream->cancel_fci.object);
    }

    RETURN_TRUE;
}
