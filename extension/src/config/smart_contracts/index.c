/*
 * =========================================================================
 * FILENAME:   src/config/smart_contracts/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Any sufficiently advanced technology is indistinguishable
 *             from magic." – Arthur C. Clarke
 *
 * PURPOSE:
 * This file implements the lifecycle (init/shutdown) functions for the
 * Smart‑Contracts configuration sub‑module.  It orchestrates the loading of
 * hard‑coded defaults and the registration of the module‑specific php.ini
 * directives so that they can override those defaults during MINIT.
 * =========================================================================
 */

#include "include/config/smart_contracts/index.h"
#include "include/config/smart_contracts/default.h"
#include "include/config/smart_contracts/ini.h"

#include "php.h"

/**
 * --------------------------------------------------------------------------
 *  Public API – called by the master dispatcher in quicpro_ini.c
 * --------------------------------------------------------------------------
 */

/**
 * @brief Initializes the Smart‑Contracts configuration module.
 *
 * Order of operations:
 *   1. Load conservative, hard‑coded defaults that are always safe.
 *   2. Register php.ini handlers so that system administrators can override
 *      those defaults at startup.
 */
void qp_config_smart_contracts_init(void)
{
    /* Step 1 – Safe baseline. */
    qp_config_smart_contracts_defaults_load();

    /* Step 2 – Allow php.ini to overwrite the baseline. */
    qp_config_smart_contracts_ini_register();
}

/**
 * @brief Shuts down the Smart‑Contracts configuration module.
 *
 * Performs the inverse operation of ::qp_config_smart_contracts_init and
 * cleans up any resources allocated by the INI layer.
 */
void qp_config_smart_contracts_shutdown(void)
{
    qp_config_smart_contracts_ini_unregister();
}
