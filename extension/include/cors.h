/*
 * =========================================================================
 * FILENAME:   include/cors.h
 * MODULE:     quicpro_async: Native CORS Handling
 * =========================================================================
 *
 * PURPOSE:
 * This header file defines the public interface for the native, C-level
 * CORS (Cross-Origin Resource Sharing) module. This module provides a
 * high-performance, configurable mechanism for enforcing CORS policies
 * directly within the extension, before a request ever reaches the PHP
 * userland.
 *
 * PHILOSOPHY:
 * The implementation follows a hybrid approach to offer both out-of-the-box
 * convenience and maximum flexibility:
 *
 * 1.  **Native Handling for Common Cases:** For simple, static policies
 * (e.g., allowing a known list of domains or a wildcard '*'), the
 * entire logic, including preflight OPTIONS requests, is handled
 * by the C-core for maximum performance. This is configured via
 * php.ini or the `Quicpro\Config` object.
 *
 * 2.  **Userland Fallback for Complex Cases:** The native handler can be
 * explicitly disabled on a per-session basis. This passes the full
 * request up to the PHP application, allowing developers to implement
 * complex, dynamic CORS logic (e.g., based on database lookups)
 * when necessary.
 *
 * INTEGRATION:
 * The main server request processing logic calls the central function
 * `quicpro_cors_handle_request()` early in the request lifecycle. This
 * function acts as a gatekeeper, deciding whether to handle the request,
 * reject it, or pass it through to the application logic.
 *
 * =========================================================================
 */

#ifndef QUICPRO_CORS_H
#define QUICPRO_CORS_H

#include "php.h"
#include "php_quicpro.h"
#include "session.h"
#include "http3.h"

// Forward declaration of the session struct to avoid circular dependencies.
struct quicpro_session_s;

/**
 * @brief Represents the parsed CORS configuration.
 *
 * This struct holds the processed CORS policy derived from either php.ini or
 * a Quicpro\Config object. It is embedded within the main quicpro_cfg_t
 * struct to keep the configuration modular.
 */
typedef struct quicpro_cors_config_s {
    /**
     * @brief A boolean flag to indicate if the native CORS handler is enabled.
     * This will be false if 'cors_allowed_origins' is explicitly set to false
     * or if no origins are configured at all.
     */
    bool enabled;

    /**
     * @brief A boolean flag for the wildcard '*' case.
     * If true, any origin is allowed, and the Origin header from the request
     * will be echoed back in the 'Access-Control-Allow-Origin' response header.
     */
    bool allow_all_origins;

    /**
     * @brief The number of specific origins in the allowed_origins array.
     */
    int num_allowed_origins;

    /**
     * @brief A dynamically allocated array of strings, where each string
     * is a specific origin that is permitted (e.g., "https://my-app.com").
     */
    char **allowed_origins;

} quicpro_cors_config_t;


/**
 * @brief Defines the possible outcomes of the CORS handler function.
 *
 * This enum is used as the return type for quicpro_cors_handle_request()
 * to signal to the calling function (the main request processor) how to proceed.
 */
typedef enum {
    /**
     * The CORS handler was disabled for this session (e.g., config was `false`).
     * The request was not touched. The calling function should proceed with
     * its normal logic, allowing PHP userland to handle CORS if desired.
     */
    QP_CORS_PASSTHROUGH,

    /**
     * The request's Origin was checked and is valid according to the policy.
     * The necessary 'Access-Control-Allow-Origin' header has been prepared
     * and added to the session's response context. The calling function should
     * proceed with processing the request.
     */
    QP_CORS_REQUEST_ALLOWED,

    /**
     * The request was an OPTIONS preflight request that was successfully handled.
     * A '204 No Content' response with the appropriate CORS headers has already
     * been sent on the stream. The calling function should stop processing
     * this request and close the stream.
     */
    QP_CORS_REQUEST_HANDLED_AND_FINISHED,

    /**
     * The request's Origin is not permitted by the policy. A '403 Forbidden'
     * response has already been sent on the stream. The calling function should
     * stop processing and close the stream.
     */
    QP_CORS_REQUEST_FORBIDDEN,

} quicpro_cors_status_t;


/**
 * @brief The primary entry point for the CORS module.
 *
 * This function is called early in the request processing lifecycle. It inspects
 * the request headers and the session's CORS configuration to determine if the
 * cross-origin request is allowed. It will handle OPTIONS preflight requests
 * directly and prepare the necessary headers for other requests.
 *
 * @param session The active session object, which contains the configuration
 * and the means to send a response.
 * @param request A struct representing the incoming HTTP request, which contains
 * the headers (specifically the 'Origin' header).
 * @return A quicpro_cors_status_t enum value indicating the outcome.
 */
quicpro_cors_status_t quicpro_cors_handle_request(struct quicpro_session_s *session, quicpro_http_request_t *request);

/**
 * @brief Initializes the CORS configuration struct.
 * @param cors_cfg A pointer to the quicpro_cors_config_t to initialize.
 */
void quicpro_cors_config_init(quicpro_cors_config_t *cors_cfg);

/**
 * @brief Frees any memory allocated by the CORS configuration struct.
 * @param cors_cfg A pointer to the quicpro_cors_config_t to destroy.
 */
void quicpro_cors_config_dtor(quicpro_cors_config_t *cors_cfg);


#endif // QUICPRO_CORS_H