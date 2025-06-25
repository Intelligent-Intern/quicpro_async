/*
 * =========================================================================
 * FILENAME:   src/config/semantic_geometry/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    And of the objects which are being carried in like manner
 * they would only see the shadows?
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the `semantic_geometry` module.
 * =========================================================================
 */

#include "include/config/semantic_geometry/config.h"
#include "include/config/semantic_geometry/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers that we will create next */
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_double_range.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_semantic_geometry_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /* Step 1: Enforce the global security policy. */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    /* Step 2: Ensure we have an array. */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    /*
     * Step 3: Iterate, validate, and apply each setting using dedicated
     * validation helpers.
     */
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }

        if (zend_string_equals_literal(key, "default_vector_dimensions")) {
            if (qp_validate_positive_long(value, &quicpro_semantic_geometry_config.default_vector_dimensions) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "calculation_precision")) {
            const char *allowed[] = {"float32", "float64", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_semantic_geometry_config.calculation_precision) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "convex_hull_algorithm")) {
            const char *allowed[] = {"qhull", "gift_wrapping", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_semantic_geometry_config.convex_hull_algorithm) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "point_in_polytope_algorithm")) {
            const char *allowed[] = {"ray_casting", "barycentric", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_semantic_geometry_config.point_in_polytope_algorithm) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "hausdorff_distance_algorithm")) {
            const char *allowed[] = {"exact", "approximated", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_semantic_geometry_config.hausdorff_distance_algorithm) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "spiral_search_step_size")) {
            if (qp_validate_double_range(value, 0.0, 1.0, &quicpro_semantic_geometry_config.spiral_search_step_size) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "core_consolidation_threshold")) {
            if (qp_validate_double_range(value, 0.0, 1.0, &quicpro_semantic_geometry_config.core_consolidation_threshold) != SUCCESS) {
                return FAILURE;
            }
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
