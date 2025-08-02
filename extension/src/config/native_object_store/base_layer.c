/*
 * =========================================================================
 * FILENAME:   src/config/native_object_store/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It's a dangerous business, Frodo, going out your door.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_native_object_store_config` global variable.
 * =========================================================================
 */
#include "include/config/native_object_store/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_native_object_store_config_t quicpro_native_object_store_config;
