/*
 * =========================================================================
 * FILENAME:   include/config/tcp_transport/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    When you look into the abyss, the abyss looks into you.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the TCP Transport layer.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_TCP_TRANSPORT_DEFAULT_H
#define QUICPRO_CONFIG_TCP_TRANSPORT_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_tcp_transport_defaults_load(void);

#endif /* QUICPRO_CONFIG_TCP_TRANSPORT_DEFAULT_H */
