/*
 * =========================================================================
 * FILENAME:   src/config/security_and_traffic/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Never gonna give you up...
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the `security_and_traffic`
 * configuration module, as declared in its corresponding `index.h` file.
 *
 * ARCHITECTURE:
 * This is the conductor. It dictates the precise order of operations for
 * loading the configuration for this module, ensuring maximum security
 * and clarity. The sequence is as follows:
 *
 * 1. `qp_config_security_defaults_load()`:
 * Loads hardcoded, secure-by-default values into the local config struct.
 * At this point, userland override is implicitly forbidden.
 *
 * 2. `qp_config_security_ini_register()`:
 * Registers the INI handlers. The Zend Engine then immediately calls
 * the `OnUpdate...` handlers which load and validate values from
 * `php.ini` into the local config struct.
 *
 * 3. `qp_config_security_policy_apply()`:
 * This is a distinct, explicit step. A dedicated function reads the
 * now-populated local config struct and makes the final, authoritative
 * decision to set the global `is_userland_override_allowed` flag.
 *
 * 4. Userland `Quicpro\Config` Object Handling (Post-MINIT):
 * The logic for handling runtime overrides from PHP code is separate
 * and will only proceed if the global flag allows it.
 * =========================================================================
 */

#include "include/config/security_and_traffic/index.h"
#include "include/config/security_and_traffic/default.h"
#include "include/config/security_and_traffic/ini.h"
#include "include/config/security_and_traffic/policy.h" /* For applying the security policy */
#include "php.h"

/**
 * @brief Initializes the Security & Traffic configuration module.
 * @details This function orchestrates the strict, multi-stage loading sequence
 * for all security-related settings, ensuring a secure-by-default posture.
 */
void qp_config_security_and_traffic_init(void)
{
    /* Step 1: Populate the config struct with hardcoded, safe defaults. */
    qp_config_security_defaults_load();

    /*
     * Step 2: Register the php.ini handlers. The Zend Engine will now
     * automatically parse the ini file, calling our OnUpdate handlers
     * which validate and load the values into our local config struct.
     */
    qp_config_security_ini_register();

    /*
     * Step 3: Explicitly apply the security policy. This function reads
     * the state of the local config struct and makes the final decision
     * about the global `is_userland_override_allowed` flag.
     */
    qp_config_security_policy_apply();
}

/**
 * @brief Shuts down the Security & Traffic configuration module.
 * @details This function calls the `qp_config_security_ini_unregister`
 * function to instruct the Zend Engine to remove this module's `php.ini`
 * entries during the shutdown sequence, freeing associated resources.
 */
void qp_config_security_and_traffic_shutdown(void)
{
    qp_config_security_ini_unregister();
}
