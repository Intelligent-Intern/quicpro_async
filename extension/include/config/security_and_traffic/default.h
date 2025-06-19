/*
 * =========================================================================
 * FILENAME:   include/config/security_and_traffic/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Never gonna let you down...
 *
 * PURPOSE:
 * This header file declares the function responsible for loading the
 * module's hardcoded, secure-by-default values.
 *
 * ARCHITECTURE:
 * The function declared here is the first one called in the module's
 * initialization sequence. It establishes a known, safe baseline before
 * any user-provided configuration is processed.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SECURITY_DEFAULT_H
#define QUICPRO_CONFIG_SECURITY_DEFAULT_H

/**
 * @brief Loads the hardcoded, secure-by-default values into the module's
 * config struct. This is the first step in the configuration load sequence.
 */
void qp_config_security_defaults_load(void);

#endif /* QUICPRO_CONFIG_SECURITY_DEFAULT_H */
