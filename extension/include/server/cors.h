/* extension/include/server/cors.h */

#ifndef QUICPRO_SERVER_CORS_H
#define QUICPRO_SERVER_CORS_H

#include <php.h>

/**
 * @file extension/include/server/cors.h
 * @brief Declarations for the native Cross-Origin Resource Sharing (CORS) handler.
 *
 * This header defines the internal interfaces for the C-level processing of
 * CORS requests. Instead of being exposed as a direct PHP function, the native
 * CORS handling is automatically enabled and configured via the `Quicpro\Config`
 * object passed to a server instance.
 *
 * This design ensures that CORS policies are enforced with maximum performance
 * at the lowest possible layer, handling `OPTIONS` preflight requests and adding
 * necessary `Access-Control-*` headers before the request even reaches the
 * PHP userland.
 */

/**
 * @brief (Internal) Applies the configured CORS policy to an incoming request.
 *
 * This function is invoked internally by the server's core request processing
 * loop for each new request. It inspects the `Origin` header and validates it
 * against the `cors_allowed_origins` policy defined in the session's
 * configuration.
 *
 * If the request is a valid CORS preflight (`OPTIONS` method), this function
 * will handle it by sending an immediate 204 No Content response with the
 * appropriate CORS headers and stopping further processing. For actual requests
 * from a permitted origin, it ensures the `Access-Control-Allow-Origin` header
 * is added to the final response.
 *
 * @param session_resource The resource of the current session/request.
 * @return An internal status flag indicating whether the request is a preflight
 * that has been handled (`CORS_PREFLIGHT_HANDLED`), is a valid CORS request
 * that can proceed (`CORS_OK`), or is a forbidden request (`CORS_FORBIDDEN`).
 */
// This function is for internal C-level use and is not exported to PHP.
// Its behavior is controlled entirely by the `cors_allowed_origins`
// setting in the `Quicpro\Config` object.

#endif // QUICPRO_SERVER_CORS_H
