/*
 * =========================================================================
 * FILENAME:   src/config/native_object_store/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You step onto the road, and if you don't keep your feet,
 * there's no knowing where you might be swept off to.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the Native Object Store
 * configuration module.
 * =========================================================================
 */

#include "include/config/native_object_store/index.h"
#include "include/config/native_object_store/default.h"
#include "include/config/native_object_store/ini.h"
#include "php.h"

/**
 * @brief Initializes the Native Object Store configuration module.
 */
void qp_config_native_object_store_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_native_object_store_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_native_object_store_ini_register();
}

/**
 * @brief Shuts down the Native Object Store configuration module.
 */
void qp_config_native_object_store_shutdown(void)
{
    qp_config_native_object_store_ini_unregister();
}
