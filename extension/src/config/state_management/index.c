/*
 * =========================================================================
 * FILENAME:   src/config/state_management/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Growth for the sake of growth is the ideology of a cancer cell." — Edward Abbey
 *
 * PURPOSE:
 *   Provides the lifecycle entry points (init/shutdown) for the State‑
 *   Management configuration module.  It is invoked by the master
 *   dispatcher during module bootstrap and teardown.
 * =========================================================================
 */

#include "include/config/state_management/index.h"
#include "include/config/state_management/default.h"
#include "include/config/state_management/ini.h"
#include "php.h"

void qp_config_state_management_init(void)
{
    /* 1. Load safe defaults */
    qp_config_state_management_defaults_load();

    /* 2. Register INI entries */
    qp_config_state_management_ini_register();
}

void qp_config_state_management_shutdown(void)
{
    qp_config_state_management_ini_unregister();
}
