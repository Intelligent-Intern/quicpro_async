/*
 * =========================================================================
 * FILENAME:   include/config/state_management/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What is the most resilient parasite? A bacteria? A virus?
 * An intestinal worm? An idea.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `state_management` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the framework's generic,
 * persistent state manager backend.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_STATE_MANAGEMENT_BASE_H
#define QUICPRO_CONFIG_STATE_MANAGEMENT_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_state_management_config_t {
    char *default_backend;
    char *default_uri;

} qp_state_management_config_t;

/* The single instance of this module's configuration data */
extern qp_state_management_config_t quicpro_state_management_config;

#endif /* QUICPRO_CONFIG_STATE_MANAGEMENT_BASE_H */
