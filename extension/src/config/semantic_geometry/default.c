/*
 * =========================================================================
 * FILENAME:   src/config/semantic_geometry/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You have shown me a strange image, and they are strange
 * prisoners.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the `semantic_geometry` configuration struct.
 * =========================================================================
 */

#include "include/config/semantic_geometry/default.h"
#include "include/config/semantic_geometry/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_semantic_geometry_defaults_load(void)
{
    quicpro_semantic_geometry_config.default_vector_dimensions = 768;
    quicpro_semantic_geometry_config.calculation_precision = pestrdup("float64", 1);
    quicpro_semantic_geometry_config.convex_hull_algorithm = pestrdup("qhull", 1);
    quicpro_semantic_geometry_config.point_in_polytope_algorithm = pestrdup("ray_casting", 1);
    quicpro_semantic_geometry_config.hausdorff_distance_algorithm = pestrdup("exact", 1);
    quicpro_semantic_geometry_config.spiral_search_step_size = 0.1;
    quicpro_semantic_geometry_config.core_consolidation_threshold = 0.95;
}
