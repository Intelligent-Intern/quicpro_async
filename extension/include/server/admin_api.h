/* extension/include/server/admin_api.h */

#ifndef QUICPRO_SERVER_ADMIN_API_H
#define QUICPRO_SERVER_ADMIN_API_H

#include <php.h>

/**
 * @file extension/include/server/admin_api.h
 * @brief Public API declarations for the dynamic Admin API Server.
 *
 * This header defines the interface for the high-security Admin API, which
 * enables dynamic, zero-downtime reconfiguration of a running Quicpro server
 * instance. Use cases include live-reloading of TLS certificates, updating
 * routing tables, or adjusting security policies without service interruption.
 *
 * Authentication for this endpoint is strictly enforced via Mutual TLS (mTLS)
 * to ensure that only authorized clients can perform administrative actions.
 */

/**
 * @brief Initializes and starts the Admin API listener.
 *
 * This function launches a dedicated server endpoint that listens for
 * administrative commands, typically structured as JSON payloads. The endpoint's
 * configuration (e.g., listening port, trusted client CAs) is derived from
 * the `quicpro.admin_api_*` directives in php.ini or a specific `Quicpro\Config`
 * object.
 *
 * This listener runs within the main server's process but operates on a
 * separate, non-public port, and its sole purpose is to receive and apply
 * configuration changes to its parent server instance.
 *
 * @param target_server_resource The resource of the main server instance that
 * administrative commands will be applied to. This parameter is mandatory.
 * @param config_resource A PHP resource representing a `Quicpro\Config` object
 * specifically for the Admin API. This configuration MUST enforce mTLS by
 * including `verify_peer`, `ca_file`, `cert_file`, and `key_file` directives.
 * @return TRUE on successful initialization of the Admin API listener.
 * FALSE on failure, throwing a `Quicpro\Exception\ServerException` or a
 * more specific exception if the configuration is insecure or invalid.
 */
PHP_FUNCTION(quicpro_admin_api_listen);

#endif // QUICPRO_SERVER_ADMIN_API_H
