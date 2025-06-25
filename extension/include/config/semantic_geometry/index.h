/*
 * =========================================================================
 * FILENAME:   include/config/semantic_geometry/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    They have been there from their childhood, and have their
 * legs and necks chained so that they cannot move.
 *
 * PURPOSE:
 * This header file declares the public C-API for the Semantic Geometry
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SEMANTIC_GEOMETRY_INDEX_H
#define QUICPRO_CONFIG_SEMANTIC_GEOMETRY_INDEX_H

/**
 * @brief Initializes the Semantic Geometry configuration module.
 */
void qp_config_semantic_geometry_init(void);

/**
 * @brief Shuts down the Semantic Geometry configuration module.
 */
void qp_config_semantic_geometry_shutdown(void);

#endif /* QUICPRO_CONFIG_SEMANTIC_GEOMETRY_INDEX_H */
