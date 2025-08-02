/*
 * =========================================================================
 * FILENAME:   src/config/state_management/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "All stable processes we shall predict. All unstable processes
 * we shall control." — John von Neumann
 *
 * PURPOSE:
 *   Loads the hard‑coded, conservative defaults for the State‑Management
 *   configuration struct. These values are guaranteed to be safe on any
 *   platform even in the absence of a customised php.ini.
 * =========================================================================
 */

#include "include/config/state_management/default.h"
#include "include/config/state_management/base_layer.h"
#include "php.h"

void qp_config_state_management_defaults_load(void)
{
    quicpro_state_mgmt_config.default_backend = pestrdup("memory", 1);
    quicpro_state_mgmt_config.default_uri     = pestrdup("",       1);
}
