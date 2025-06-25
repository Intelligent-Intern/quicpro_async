/*
 * =========================================================================
 * FILENAME:   include/config/quic_transport/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Spared no expense.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the QUIC Transport layer.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_QUIC_TRANSPORT_DEFAULT_H
#define QUICPRO_CONFIG_QUIC_TRANSPORT_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_quic_transport_defaults_load(void);

#endif /* QUICPRO_CONFIG_QUIC_TRANSPORT_DEFAULT_H */
