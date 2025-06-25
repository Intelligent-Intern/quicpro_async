/*
 * =========================================================================
 * FILENAME:   include/config/semantic_geometry/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Behold! Human beings living in an underground den.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `semantic_geometry` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the C-level geometric
 * computation engine, which handles operations on polytopes and vectors.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_SEMANTIC_GEOMETRY_BASE_H
#define QUICPRO_CONFIG_SEMANTIC_GEOMETRY_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_semantic_geometry_config_t {
    /* The default number of dimensions for new vector spaces. */
    zend_long default_vector_dimensions;

    /* The floating point precision to use for geometric calculations. */
    char *calculation_precision; /* e.g., "float32", "float64" */

    /* The algorithm to use for convex hull calculations. */
    char *convex_hull_algorithm; /* e.g., "qhull", "gift_wrapping" */

    /* The default algorithm for checking if a point is inside a polytope. */
    char *point_in_polytope_algorithm; /* e.g., "ray_casting", "barycentric" */

    /* The default algorithm for calculating the Hausdorff distance. */
    char *hausdorff_distance_algorithm; /* e.g., "exact", "approximated" */

    /* The default step size for spiral path exploration. */
    double spiral_search_step_size;

    /* The resonance threshold required to move a concept from periphery to core. */
    double core_consolidation_threshold;

} qp_semantic_geometry_config_t;

/* The single instance of this module's configuration data */
extern qp_semantic_geometry_config_t quicpro_semantic_geometry_config;

#endif /* QUICPRO_CONFIG_SEMANTIC_GEOMETRY_BASE_H */
