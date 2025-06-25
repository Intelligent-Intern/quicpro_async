/*
 * =========================================================================
 * FILENAME:   include/config/iibin/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    In the beginning the Universe was created. This has made a
 * lot of people very angry and been widely regarded as a bad move.
 *
 * PURPOSE:
 * This header file declares the public C-API for the IIBIN serialization
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_IIBIN_INDEX_H
#define QUICPRO_CONFIG_IIBIN_INDEX_H

/**
 * @brief Initializes the IIBIN Serialization configuration module.
 */
void qp_config_iibin_init(void);

/**
 * @brief Shuts down the IIBIN Serialization configuration module.
 */
void qp_config_iibin_shutdown(void);

#endif /* QUICPRO_CONFIG_IIBIN_INDEX_H */
