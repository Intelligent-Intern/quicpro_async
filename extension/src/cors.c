/*
 * =========================================================================
 * FILENAME:   src/cors.c
 * MODULE:     quicpro_async: Native CORS Handling Implementation
 * =========================================================================
 *
 * PURPOSE:
 * This file contains the C-level implementation for the native, high-
 * performance CORS (Cross-Origin Resource Sharing) module. It provides the
 * logic for the functions declared in `cors.h`.
 *
 * The core function, `quicpro_cors_handle_request`, acts as a gatekeeper
 * for incoming HTTP requests. It efficiently handles CORS preflight
 * (OPTIONS) requests and validates the `Origin` header for all other
 * requests against a pre-configured policy, all before the request is
 * passed to the PHP userland.
 *
 * This C-level handling significantly improves performance for common CORS
 * scenarios and provides a robust, out-of-the-box security feature for
 * the framework.
 *
 * =========================================================================
 */

#include "php_quicpro.h"
#include "cors.h"
#include "config.h" // To access the cors_config within the main config
#include "session.h"
#include "http3.h"

#include <string.h>

/**
 * Initializes a CORS configuration struct to its default (disabled) state.
 */
void quicpro_cors_config_init(quicpro_cors_config_t *cors_cfg)
{
    if (!cors_cfg) return;

    cors_cfg->enabled = false;
    cors_cfg->allow_all_origins = false;
    cors_cfg->num_allowed_origins = 0;
    cors_cfg->allowed_origins = NULL;
}

/**
 * Frees all dynamically allocated memory within a CORS configuration struct.
 */
void quicpro_cors_config_dtor(quicpro_cors_config_t *cors_cfg)
{
    if (!cors_cfg || !cors_cfg->allowed_origins) {
        return;
    }

    for (int i = 0; i < cors_cfg->num_allowed_origins; i++) {
        if (cors_cfg->allowed_origins[i]) {
            efree(cors_cfg->allowed_origins[i]);
        }
    }
    efree(cors_cfg->allowed_origins);

    // Reset to initial state
    quicpro_cors_config_init(cors_cfg);
}

/**
 * Internal helper function to check if a given origin is in the allowlist.
 */
static bool is_origin_allowed(const quicpro_cors_config_t *cors_cfg, const char *origin)
{
    if (cors_cfg->allow_all_origins) {
        return true;
    }

    for (int i = 0; i < cors_cfg->num_allowed_origins; i++) {
        if (strcmp(cors_cfg->allowed_origins[i], origin) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * The primary entry point for the CORS module. This function inspects the
 * request and applies the configured CORS policy.
 */
quicpro_cors_status_t quicpro_cors_handle_request(struct quicpro_session_s *session, quicpro_http_request_t *request)
{
    // Retrieve the CORS configuration associated with this session.
    // Assumes `quicpro_cfg_t` is a member of `quicpro_session_t`
    // and `quicpro_cors_config_t` is a member of `quicpro_cfg_t`.
    const quicpro_cors_config_t *cors_cfg = &session->config->cors;

    // If the native handler is not enabled for this session, do nothing.
    if (!cors_cfg || !cors_cfg->enabled) {
        return QP_CORS_PASSTHROUGH;
    }

    // Attempt to get the Origin header from the request.
    // This uses a conceptual helper function on the request object.
    const char *origin = quicpro_http_request_get_header(request, "origin");

    // If there is no Origin header, it is not a CORS request. Let it pass through.
    if (!origin) {
        return QP_CORS_PASSTHROUGH;
    }

    // Check if the provided origin is allowed by the current policy.
    bool origin_is_allowed = is_origin_allowed(cors_cfg, origin);

    // If the origin is not allowed, send a 403 Forbidden response and terminate.
    if (!origin_is_allowed) {
        // This uses a conceptual helper on the session to send a complete HTTP response.
        quicpro_session_send_error_response(session, request->stream_id, 403, "Forbidden Origin");
        return QP_CORS_REQUEST_FORBIDDEN;
    }

    // If the origin is allowed, check if this is an OPTIONS preflight request.
    if (quicpro_http_request_get_method(request) == QP_HTTP_METHOD_OPTIONS) {
        // This is a preflight request. We handle it completely and send a 204 response.
        quicpro_h3_header headers[] = {
            { .name = (uint8_t *)"access-control-allow-origin",   .name_len = 27, .value = (uint8_t *)origin, .value_len = strlen(origin) },
            { .name = (uint8_t *)"access-control-allow-methods",  .name_len = 28, .value = (uint8_t *)"GET, POST, OPTIONS", .value_len = 18 },
            { .name = (uint8_t *)"access-control-allow-headers",  .name_len = 28, .value = (uint8_t *)"Authorization, Content-Type", .value_len = 29 },
            { .name = (uint8_t *)"access-control-max-age",        .name_len = 22, .value = (uint8_t *)"86400", .value_len = 5 },
        };

        quicpro_session_send_http_response(session, request->stream_id, 204, headers, 4, NULL, 0);
        return QP_CORS_REQUEST_HANDLED_AND_FINISHED;
    }

    // If it's an actual request (e.g., GET, POST) from an allowed origin,
    // we need to add the ACAO header to the eventual response. We do this by
    // adding it to a response header context on the session.
    const char *origin_to_echo = cors_cfg->allow_all_origins ? origin : cors_cfg->allowed_origins[0]; // Or more complex logic

    // Uses a conceptual helper to stage a header for the eventual response.
    quicpro_session_add_response_header(session, "access-control-allow-origin", origin_to_echo);

    return QP_CORS_REQUEST_ALLOWED;
}