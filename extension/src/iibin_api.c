/*
 * iibin_api.c – PHP-exponierte Wrapper
 * ====================================
 *
 * Bindet die low-level Implementierung an Zend.
 */
#include "php_quicpro.h"
#include "iibin.h"
#include "iibin_internal.h"

/* ---------- Forward-Deklarationen ---------- */
PHP_FUNCTION(quicpro_iibin_define_enum);
PHP_FUNCTION(quicpro_iibin_define_schema);
PHP_FUNCTION(quicpro_iibin_encode);
PHP_FUNCTION(quicpro_iibin_decode);
PHP_FUNCTION(quicpro_iibin_is_defined);
PHP_FUNCTION(quicpro_iibin_is_schema_defined);
PHP_FUNCTION(quicpro_iibin_is_enum_defined);
PHP_FUNCTION(quicpro_iibin_get_defined_schemas);
PHP_FUNCTION(quicpro_iibin_get_defined_enums);

/* ---------- ArgInfo-Import (aus php_quicpro_arginfo.h) ---------- */
#include "php_quicpro_arginfo.h"

/* ---------- Method-Table ---------- */
static const zend_function_entry iibin_methods[] = {
    PHP_FE(quicpro_iibin_define_enum,         arginfo_quicpro_iibin_define_enum)
    PHP_FE(quicpro_iibin_define_schema,       arginfo_quicpro_iibin_define_schema)
    PHP_FE(quicpro_iibin_encode,              arginfo_quicpro_iibin_encode)
    PHP_FE(quicpro_iibin_decode,              arginfo_quicpro_iibin_decode)
    PHP_FE(quicpro_iibin_is_defined,          arginfo_quicpro_iibin_is_defined)
    PHP_FE(quicpro_iibin_is_schema_defined,   arginfo_quicpro_iibin_is_schema_defined)
    PHP_FE(quicpro_iibin_is_enum_defined,     arginfo_quicpro_iibin_is_enum_defined)
    PHP_FE(quicpro_iibin_get_defined_schemas, arginfo_quicpro_iibin_get_defined_schemas)
    PHP_FE(quicpro_iibin_get_defined_enums,   arginfo_quicpro_iibin_get_defined_enums)
    PHP_FE_END
};

/* ---------- MINIT / MSHUTDOWN ---------- */
zend_class_entry *quicpro_ce_iibin;

PHP_MINIT_FUNCTION(quicpro_iibin)
{
    /* Registries */
    if (quicpro_iibin_registries_init() != SUCCESS) return FAILURE;

    /* PHP-Klasse (static-only) */
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, "Quicpro", "IIBIN", iibin_methods);
    quicpro_ce_iibin = zend_register_internal_class(&ce);
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(quicpro_iibin)
{
    quicpro_iibin_registries_shutdown();
    return SUCCESS;
}

/* ---------- Modul-Eintrag ---------- */
zend_module_entry quicpro_iibin_module_entry = {
    STANDARD_MODULE_HEADER,
    "quicpro_iibin",      /* ext name  */
    NULL,                 /* functions (keine globalen) */
    PHP_MINIT(quicpro_iibin),
    PHP_MSHUTDOWN(quicpro_iibin),
    NULL, NULL, NULL,
    PHP_QUICPRO_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_QUICPRO_IIBIN
ZEND_GET_MODULE(quicpro_iibin)
#endif
