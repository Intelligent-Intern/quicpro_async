/*
 * =========================================================================
 * FILENAME:   include/config/ssh_over_quic/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    For someone who was never meant for this world, I must confess
 * I'm suddenly having a hard time leaving it.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the SSH over QUIC gateway.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SSH_OVER_QUIC_DEFAULT_H
#define QUICPRO_CONFIG_SSH_OVER_QUIC_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_ssh_over_quic_defaults_load(void);

#endif /* QUICPRO_CONFIG_SSH_OVER_QUIC_DEFAULT_H */
