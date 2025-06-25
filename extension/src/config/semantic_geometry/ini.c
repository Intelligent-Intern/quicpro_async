/*
 * =========================================================================
 * FILENAME:   src/config/semantic_geometry/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Like ourselves, I replied; and they see only their own shadows...
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the `semantic_geometry` module.
 * =========================================================================
 */

#include "include/config/semantic_geometry/ini.h"
#include "include/config/semantic_geometry/base_layer.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>
#include <zend_ini.h>

/* Custom INI update handlers would be defined here for validation */
/* For example, for 'calculation_precision', we'd check against a list */
/* of allowed values like "float32", "float64". */

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.geometry_default_vector_dimensions", "768", PHP_INI_SYSTEM, OnUpdateLong, default_vector_dimensions, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
    STD_PHP_INI_ENTRY("quicpro.geometry_calculation_precision", "float64", PHP_INI_SYSTEM, OnUpdateString, calculation_precision, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
    STD_PHP_INI_ENTRY("quicpro.geometry_convex_hull_algorithm", "qhull", PHP_INI_SYSTEM, OnUpdateString, convex_hull_algorithm, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
    STD_PHP_INI_ENTRY("quicpro.geometry_point_in_polytope_algorithm", "ray_casting", PHP_INI_SYSTEM, OnUpdateString, point_in_polytope_algorithm, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
    STD_PHP_INI_ENTRY("quicpro.geometry_hausdorff_distance_algorithm", "exact", PHP_INI_SYSTEM, OnUpdateString, hausdorff_distance_algorithm, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
    STD_PHP_INI_ENTRY("quicpro.geometry_spiral_search_step_size", "0.1", PHP_INI_SYSTEM, OnUpdateReal, spiral_search_step_size, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
    STD_PHP_INI_ENTRY("quicpro.geometry_core_consolidation_threshold", "0.95", PHP_INI_SYSTEM, OnUpdateReal, core_consolidation_threshold, qp_semantic_geometry_config_t, quicpro_semantic_geometry_config)
PHP_INI_END()

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_semantic_geometry_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_semantic_geometry_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
