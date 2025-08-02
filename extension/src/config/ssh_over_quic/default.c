/*
 * =========================================================================
 * FILENAME:   src/config/ssh_over_quic/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "There is no spoon."
 *
 * PURPOSE:
 *   Loads compile‑time safe defaults into the SSH‑over‑QUIC configuration
 *   structure. These values are enforced before any php.ini or userland
 *   overrides are processed, guaranteeing a deterministic baseline.
 * =========================================================================
 */

#include "include/config/ssh_over_quic/default.h"
#include "include/config/ssh_over_quic/base_layer.h"
#include "php.h"

void qp_config_ssh_over_quic_defaults_load(void)
{
    /* --- Gateway Listener Configuration ------------------------------- */
    quicpro_ssh_quic_config.gateway_enable            = false;
    quicpro_ssh_quic_config.listen_host               = pestrdup("0.0.0.0", 1);
    quicpro_ssh_quic_config.listen_port               = 2222;

    /* --- Default Upstream Target -------------------------------------- */
    quicpro_ssh_quic_config.default_target_host       = pestrdup("127.0.0.1", 1);
    quicpro_ssh_quic_config.default_target_port       = 22;
    quicpro_ssh_quic_config.target_connect_timeout_ms = 5000;

    /* --- Authentication & Target Mapping ------------------------------ */
    quicpro_ssh_quic_config.gateway_auth_mode         = pestrdup("mtls", 1); /* "mtls" | "mcp_token" */
    quicpro_ssh_quic_config.mcp_auth_agent_uri        = pestrdup("", 1);
    quicpro_ssh_quic_config.target_mapping_mode       = pestrdup("static", 1); /* "static" | "user_profile" */
    quicpro_ssh_quic_config.user_profile_agent_uri    = pestrdup("", 1);

    /* --- Session Control & Logging ------------------------------------ */
    quicpro_ssh_quic_config.idle_timeout_sec          = 1800;
    quicpro_ssh_quic_config.log_session_activity      = true;
}
