/*
 * src/iibin.c – PHP Bindings & Lifecycle for the Quicpro\IIBIN Module
 * =====================================================================
 *
 * This file acts as the primary bridge between the Zend Engine and the C-native
 * IIBIN serialization module. It is responsible for:
 *
 * 1. Defining the `Quicpro\IIBIN` PHP class and its static methods.
 * 2. Creating the `zend_function_entry` list which maps the PHP class methods
 * to their corresponding C `PHP_FUNCTION` implementations.
 * 3. Providing module-specific lifecycle hooks (MINIT and MSHUTDOWN) to initialize
 * and destroy the global schema and enum registries, ensuring proper
 * resource management.
 *
 * The actual logic for schema management, encoding, and decoding resides in their
 * respective .c files (iibin_schema.c, iibin_encoding.c, iibin_decoding.c).
 */

#include "php_quicpro.h"
#include "iibin.h"
#include "iibin_internal.h"
#include "cancel.h" /* For error throwing helpers, if needed */


/* --- Argument Information (arginfo) --- */
/*
 * This section defines the function signatures as PHP's reflection and
 * argument parsing system understands them. For a clean implementation,
 * these definitions would typically reside in php_quicpro_arginfo.h and be
 * included here. For completeness of this file as a module, they are shown here.
 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_define_enum, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, enumName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, enumValues, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_define_schema, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, schemaDefinition, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_encode, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, phpData, 0, 0) /* Can be array or object */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_decode, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, binaryData, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, decodeAsObject, IS_FALSE, 1) /* Optional bool */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_is_defined, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_is_schema_defined, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_is_enum_defined, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, enumName, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_get_defined_schemas, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_iibin_get_defined_enums, 0, 0, 0)
ZEND_END_ARG_INFO()


/* --- Method Table for Quicpro\IIBIN PHP class --- */
/*
 * This array maps the static methods of the `Quicpro\IIBIN` PHP class
 * to their corresponding C function implementations.
 */
static const zend_function_entry quicpro_iibin_methods[] = {
    PHP_ME(QuicproIIBIN, defineEnum,         arginfo_quicpro_iibin_define_enum,         ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, defineSchema,       arginfo_quicpro_iibin_define_schema,       ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, encode,             arginfo_quicpro_iibin_encode,              ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, decode,             arginfo_quicpro_iibin_decode,              ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, isDefined,          arginfo_quicpro_iibin_is_defined,          ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, isSchemaDefined,    arginfo_quicpro_iibin_is_schema_defined,   ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, isEnumDefined,      arginfo_quicpro_iibin_is_enum_defined,     ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, getDefinedSchemas,  arginfo_quicpro_iibin_get_defined_schemas, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(QuicproIIBIN, getDefinedEnums,    arginfo_quicpro_iibin_get_defined_enums,   ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};


/* --- Module Initialization for IIBIN --- */

/**
 * @var zend_class_entry *quicpro_ce_iibin
 * @brief Pointer to the class entry for the `Quicpro\IIBIN` PHP class.
 *
 * This global variable holds the handle to the registered class.
 */
zend_class_entry *quicpro_ce_iibin;

/**
 * @brief Initializes the IIBIN module during PHP's MINIT phase.
 *
 * This function registers the `Quicpro\IIBIN` class and its methods with the
 * Zend Engine. It also calls the initialization function for the global
 * schema and enum registries.
 */
void quicpro_iibin_minit(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Quicpro", "IIBIN", quicpro_iibin_methods);
    quicpro_ce_iibin = zend_register_internal_class(&ce);
    /* The IIBIN class contains only static methods, so no object handlers are needed. */

    /* Initialize the global schema and enum registries. */
    quicpro_iibin_registries_init();
}

/**
 * @brief Shuts down the IIBIN module during PHP's MSHUTDOWN phase.
 *
 * This function is responsible for calling the shutdown function for the global
 * registries, which will trigger the destructors for all compiled schemas
 * and enums, ensuring all allocated C memory is freed.
 */
void quicpro_iibin_mshutdown(void)
{
    quicpro_iibin_registries_shutdown();
}

/*
 * Note: The actual PHP_FUNCTION implementations (e.g., PHP_FUNCTION(quicpro_iibin_define_schema))
 * are located in their respective C files (iibin_schema.c, iibin_encoding.c, iibin_decoding.c).
 * This file's purpose is to define the PHP class and its method table, and to provide the
 * module lifecycle hooks (minit/mshutdown).
 */
