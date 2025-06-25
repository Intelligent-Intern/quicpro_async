/*
 * =========================================================================
 * FILENAME:   src/config/semantic_geometry/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    ...and statues and figures of animals made of wood and stone
 * and various materials, which appear over the wall?
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the `semantic_geometry`
 * configuration module.
 * =========================================================================
 */

#include "include/config/semantic_geometry/index.h"
#include "include/config/semantic_geometry/default.h"
#include "include/config/semantic_geometry/ini.h"
#include "php.h"

/**
 * @brief Initializes the Semantic Geometry configuration module.
 */
void qp_config_semantic_geometry_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_semantic_geometry_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_semantic_geometry_ini_register();
}

/**
 * @brief Shuts down the Semantic Geometry configuration module.
 */
void qp_config_semantic_geometry_shutdown(void)
{
    qp_config_semantic_geometry_ini_unregister();
}
