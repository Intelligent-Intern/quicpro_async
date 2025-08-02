/**
 * @file extension/src/server/cors.c
 * @brief Implementation of the native CORS request handler.
 *
 * This file contains the C-level logic for enforcing CORS policies. It is
 * designed to be called early in the request processing chain to handle
 * preflight requests with maximum performance and to ensure security policies
 * are applied before any application logic is executed.
 */

#include <php.h>
#include <string.h>

#include "server/cors.h"
#include "session/session.h" // For quicpro_request_t and quicpro_response_t structures
#include "config/config.h"   // For quicpro_config_t structure

/**
 * @brief Checks if a given origin string matches the allowed origins policy.
 *
 * This function performs a case-sensitive comparison of the origin against a
 * comma-separated list of allowed origins. It supports the wildcard "*" for
 * public APIs.
 *
 * @param origin The Origin header value from the incoming request.
 * @param allowed_origins A comma-separated string of allowed origins, or "*".
 * @return true if the origin is allowed, false otherwise.
 */
static bool is_origin_allowed(const char *origin, const char *allowed_origins)
{
    if (!origin || !allowed_origins) {
        return false;
    }

    if (strcmp(allowed_origins, "*") == 0) {
        return true;
    }

    const char *p = allowed_origins;
    size_t origin_len = strlen(origin);

    while (*p) {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        const char *end = strchr(p, ',');
        size_t len = (end) ? (size_t)(end - p) : strlen(p);

        // Trim trailing whitespace from the policy entry
        while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t')) {
            len--;
        }

        if (len == origin_len && strncmp(origin, p, len) == 0) {
            return true;
        }

        if (!end) {
            break;
        }
        p = end + 1;
    }

    return false;
}

/**
 * @brief (Internal) Applies the configured CORS policy to an incoming request.
 *
 * This is the core internal function called by the server's event loop. It
 * checks the Origin header, validates it, and handles preflight requests.
 */
int quicpro_internal_handle_cors(quicpro_request_t *req, quicpro_response_t *resp, quicpro_config_t *config)
{
    const char *origin = quicpro_request_get_header(req, "origin");
    if (!origin) {
        // This is not a CORS request.
        return CORS_REQUEST_OK;
    }

    if (!config->cors_allowed_origins || !is_origin_allowed(origin, config->cors_allowed_origins)) {
        // The origin is not in the allowlist.
        return CORS_REQUEST_FORBIDDEN;
    }

    // The origin is allowed. Always add the Access-Control-Allow-Origin header.
    quicpro_response_add_header(resp, "Access-Control-Allow-Origin", origin);
    // Vary header is important for caching proxies to cache responses correctly.
    quicpro_response_add_header(resp, "Vary", "Origin");

    // Handle the OPTIONS preflight request.
    if (strcmp(quicpro_request_get_method(req), "OPTIONS") == 0) {
        quicpro_response_set_status(resp, 204); // No Content

        // These headers can be made configurable in a future version if needed.
        quicpro_response_add_header(resp, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
        quicpro_response_add_header(resp, "Access-Control-Allow-Headers", "*"); // Reflect requested headers in a real implementation
        quicpro_response_add_header(resp, "Access-Control-Max-Age", "86400"); // 24 hours

        return CORS_PREFLIGHT_HANDLED;
    }

    return CORS_REQUEST_OK;
}
