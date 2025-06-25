/*
 * =========================================================================
 * FILENAME:   include/config/semantic_geometry/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    And if they were able to converse with one another, would they
 * not suppose that they were naming what was actually before them?
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the Semantic Geometry configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SEMANTIC_GEOMETRY_INI_H
#define QUICPRO_CONFIG_SEMANTIC_GEOMETRY_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_semantic_geometry_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_semantic_geometry_ini_unregister(void);

#endif /* QUICPRO_CONFIG_SEMANTIC_GEOMETRY_INI_H */
