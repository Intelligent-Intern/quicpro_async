/*
 * =========================================================================
 * FILENAME:   include/config/bare_metal_tuning/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I've seen things you people wouldn't believe...
 *
 * PURPOSE:
 * This header file declares the public C-API for the bare metal tuning
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_BARE_METAL_INDEX_H
#define QUICPRO_CONFIG_BARE_METAL_INDEX_H

/**
 * @brief Initializes the Bare Metal Tuning configuration module.
 */
void qp_config_bare_metal_tuning_init(void);

/**
 * @brief Shuts down the Bare Metal Tuning configuration module.
 */
void qp_config_bare_metal_tuning_shutdown(void);

#endif /* QUICPRO_CONFIG_BARE_METAL_INDEX_H */
