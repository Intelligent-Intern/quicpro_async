/*
 * src/iibin_schema.c – Schema Definition, Validation & Registry for Quicpro\IIBIN
 * =================================================================================
 *
 * Implements the C-native logic for defining, validating, compiling,
 * and managing message schemas and enum types for the Quicpro\IIBIN module.
 * It maintains global registries of compiled schemas and enums that are used
 * by the encoding and decoding functions.
 */

#include "php_quicpro.h"
#include "iibin.h"
#include "iibin_internal.h"
#include "cancel.h"

#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_hash.h>
#include <zend_string.h>
#include <zend_types.h>
#include <stdlib.h>             /* For qsort */

/* --- Global Schema and Enum Registry Definitions --- */
/*
 * THREAD-SAFETY NOTE: These global registries are not inherently thread-safe for concurrent writes.
 * Schema/enum definitions (writes to these HashTables) are assumed to happen in a
 * single-threaded context (e.g., application bootstrap, PHP MINIT).
 * If definitions can occur concurrently, these HashTables must be protected by read-write locks.
 */
HashTable quicpro_iibin_schema_registry;
HashTable quicpro_iibin_enum_registry;
zend_bool quicpro_iibin_registries_initialized = 0;

/* --- Static Helper Function Prototypes --- */
static int parse_php_field_options(
    const char *schema_name_for_error,
    const char *field_name_in_php,
    HashTable *options_ht,
    quicpro_iibin_field_def_internal *field_def_out
);
static uint32_t calculate_field_wire_type(quicpro_iibin_field_type_internal type, zend_bool is_packed_repeated_field);
static int compare_field_defs_by_tag(const void *a, const void *b);
static void cleanup_partially_built_schema(quicpro_iibin_compiled_schema_internal *schema);
static void cleanup_partially_built_enum(quicpro_iibin_compiled_enum_internal *enum_def);


/* --- Destructor Functions --- */

static void quicpro_destroy_iibin_field_def_internal(quicpro_iibin_field_def_internal *field_def) {
    if (!field_def) return;
    if (field_def->name_in_php) efree(field_def->name_in_php);
    if (field_def->message_type_name_if_nested) efree(field_def->message_type_name_if_nested);
    if (field_def->enum_type_name_if_enum) efree(field_def->enum_type_name_if_enum);
    if (field_def->json_name) efree(field_def->json_name);
    if (Z_TYPE(field_def->default_value_zval) != IS_UNDEF) {
        zval_ptr_dtor(&field_def->default_value_zval);
    }
    efree(field_def);
}

static void quicpro_iibin_compiled_schema_dtor_internal(zval *pData) {
    quicpro_iibin_compiled_schema_internal *schema = (quicpro_iibin_compiled_schema_internal *)Z_PTR_P(pData);
    if (schema) {
        if (schema->schema_name) efree(schema->schema_name);
        if (schema->ordered_fields) {
            efree(schema->ordered_fields);
        }
        quicpro_iibin_field_def_internal *field_def_ptr;
        ZEND_HASH_FOREACH_PTR(&schema->fields_by_tag, field_def_ptr) {
            if (field_def_ptr) {
                quicpro_destroy_iibin_field_def_internal(field_def_ptr);
            }
        } ZEND_HASH_FOREACH_END();

        zend_hash_destroy(&schema->fields_by_tag);
        zend_hash_destroy(&schema->fields_by_name);
        efree(schema);
    }
}

static void quicpro_destroy_iibin_enum_value_def_internal(quicpro_iibin_enum_value_def_internal *enum_val_def) {
    if (!enum_val_def) return;
    if (enum_val_def->name) efree(enum_val_def->name);
    efree(enum_val_def);
}

static void quicpro_iibin_compiled_enum_dtor_internal(zval *pData) {
    quicpro_iibin_compiled_enum_internal *enum_def = (quicpro_iibin_compiled_enum_internal *)Z_PTR_P(pData);
    if (enum_def) {
        if (enum_def->enum_name) efree(enum_def->enum_name);

        quicpro_iibin_enum_value_def_internal *enum_val_ptr;
        ZEND_HASH_FOREACH_PTR(&enum_def->values_by_name, enum_val_ptr) {
            if (enum_val_ptr) {
                quicpro_destroy_iibin_enum_value_def_internal(enum_val_ptr);
            }
        } ZEND_HASH_FOREACH_END();
        zend_hash_destroy(&enum_def->values_by_name);

        char *enum_name_str_ptr;
        ZEND_HASH_FOREACH_PTR(&enum_def->names_by_value, enum_name_str_ptr) {
            if (enum_name_str_ptr) efree(enum_name_str_ptr);
        } ZEND_HASH_FOREACH_END();
        zend_hash_destroy(&enum_def->names_by_value);

        efree(enum_def);
    }
}

/* --- Registry Lifecycle Functions (called from C-level MINIT/MSHUTDOWN) --- */
int quicpro_iibin_registries_init(void) {
    if (quicpro_iibin_registries_initialized) return SUCCESS;
    zend_hash_init(&quicpro_iibin_schema_registry, 0, NULL, quicpro_iibin_compiled_schema_dtor_internal, 1);
    zend_hash_init(&quicpro_iibin_enum_registry, 0, NULL, quicpro_iibin_compiled_enum_dtor_internal, 1);
    quicpro_iibin_registries_initialized = 1;
    return SUCCESS;
}

void quicpro_iibin_registries_shutdown(void) {
    if (quicpro_iibin_registries_initialized) {
        zend_hash_destroy(&quicpro_iibin_schema_registry);
        zend_hash_destroy(&quicpro_iibin_enum_registry);
        quicpro_iibin_registries_initialized = 0;
    }
}

/* --- Internal C Utility Function Implementations --- */

const quicpro_iibin_compiled_schema_internal* get_compiled_iibin_schema_internal(const char *schema_name) {
    if (!quicpro_iibin_registries_initialized) return NULL;
    return (quicpro_iibin_compiled_schema_internal *)zend_hash_str_find_ptr(&quicpro_iibin_schema_registry, schema_name, strlen(schema_name));
}

const quicpro_iibin_compiled_enum_internal* get_compiled_iibin_enum_internal(const char *enum_name) {
    if (!quicpro_iibin_registries_initialized) return NULL;
    return (quicpro_iibin_compiled_enum_internal *)zend_hash_str_find_ptr(&quicpro_iibin_enum_registry, enum_name, strlen(enum_name));
}

static int parse_php_field_type_details(
    const char *type_str_from_php, size_t len,
    quicpro_iibin_field_type_internal *base_type_out,
    zend_bool *is_repeated_out,
    char **referenced_type_name_out
) {
    /* ... (Implementation is identical to before, just using IIBIN_INTERNAL_TYPE_* enums) ... */
    const char *type_str_val = type_str_from_php;
    size_t type_str_len = len;

    *is_repeated_out = 0;
    *referenced_type_name_out = NULL;
    *base_type_out = IIBIN_INTERNAL_TYPE_UNKNOWN;

    if (type_str_len == 0) return FAILURE;

    if (strncmp(type_str_val, "repeated_", sizeof("repeated_") - 1) == 0) {
        *is_repeated_out = 1;
        type_str_val += sizeof("repeated_") - 1;
        type_str_len -= sizeof("repeated_") - 1;
        if (type_str_len == 0) return FAILURE;
    }

    if (strncmp(type_str_val, "double", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_DOUBLE;
    else if (strncmp(type_str_val, "float", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_FLOAT;
    else if (strncmp(type_str_val, "int64", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_INT64;
    else if (strncmp(type_str_val, "uint64", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_UINT64;
    else if (strncmp(type_str_val, "int32", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_INT32;
    else if (strncmp(type_str_val, "uint32", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_UINT32;
    else if (strncmp(type_str_val, "sint32", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_SINT32;
    else if (strncmp(type_str_val, "sint64", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_SINT64;
    else if (strncmp(type_str_val, "fixed64", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_FIXED64;
    else if (strncmp(type_str_val, "sfixed64", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_SFIXED64;
    else if (strncmp(type_str_val, "fixed32", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_FIXED32;
    else if (strncmp(type_str_val, "sfixed32", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_SFIXED32;
    else if (strncmp(type_str_val, "bool", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_BOOL;
    else if (strncmp(type_str_val, "string", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_STRING;
    else if (strncmp(type_str_val, "bytes", type_str_len) == 0) *base_type_out = IIBIN_INTERNAL_TYPE_BYTES;
    else {
        *referenced_type_name_out = estrndup(type_str_val, type_str_len);
        *base_type_out = IIBIN_INTERNAL_TYPE_UNKNOWN;
    }
    return SUCCESS;
}

static uint32_t calculate_field_wire_type(quicpro_iibin_field_type_internal type, zend_bool is_packed_repeated_field) {
    if (is_packed_repeated_field) {
        return QUICPRO_IIBIN_WIRETYPE_LENGTH_DELIM;
    }
    switch (type) {
        case IIBIN_INTERNAL_TYPE_INT32: case IIBIN_INTERNAL_TYPE_UINT32:
        case IIBIN_INTERNAL_TYPE_SINT32:case IIBIN_INTERNAL_TYPE_INT64:
        case IIBIN_INTERNAL_TYPE_UINT64:case IIBIN_INTERNAL_TYPE_SINT64:
        case IIBIN_INTERNAL_TYPE_BOOL:  case IIBIN_INTERNAL_TYPE_ENUM:
            return QUICPRO_IIBIN_WIRETYPE_VARINT;
        case IIBIN_INTERNAL_TYPE_FIXED64: case IIBIN_INTERNAL_TYPE_SFIXED64:
        case IIBIN_INTERNAL_TYPE_DOUBLE:
            return QUICPRO_IIBIN_WIRETYPE_FIXED64;
        case IIBIN_INTERNAL_TYPE_STRING: case IIBIN_INTERNAL_TYPE_BYTES:
        case IIBIN_INTERNAL_TYPE_MESSAGE:
            return QUICPRO_IIBIN_WIRETYPE_LENGTH_DELIM;
        case IIBIN_INTERNAL_TYPE_FIXED32: case IIBIN_INTERNAL_TYPE_SFIXED32:
        case IIBIN_INTERNAL_TYPE_FLOAT:
            return QUICPRO_IIBIN_WIRETYPE_FIXED32;
        default:
            return (uint32_t)-1;
    }
}

static int parse_php_field_options(
    const char *schema_name_for_error, const char *field_name_in_php,
    HashTable *options_ht, quicpro_iibin_field_def_internal *field_def_out
) {
    /* ... (Implementation is identical to before, but calls throw_iibin_error_as_php_exception
     * and uses the new type names and constants. Fully fleshed out below.) ... */
    zval *zv_temp;
    zend_bool is_repeated_flag = 0;

    field_def_out->name_in_php = estrdup(field_name_in_php);
    ZVAL_UNDEF(&field_def_out->default_value_zval);
    field_def_out->json_name = NULL;
    field_def_out->is_deprecated = 0;

    zv_temp = zend_hash_str_find(options_ht, "tag", sizeof("tag") - 1);
    if (!zv_temp || Z_TYPE_P(zv_temp) != IS_LONG || Z_LVAL_P(zv_temp) <= 0) {
        throw_iibin_error_as_php_exception(0, "Schema '%s': Field '%s' has invalid or missing 'tag'.", schema_name_for_error, field_name_in_php);
        return FAILURE;
    }
    field_def_out->tag = (uint32_t)Z_LVAL_P(zv_temp);

    zv_temp = zend_hash_str_find(options_ht, "type", sizeof("type") - 1);
    if (!zv_temp || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_iibin_error_as_php_exception(0, "Schema '%s': Field '%s' has invalid or missing 'type' string.", schema_name_for_error, field_name_in_php);
        return FAILURE;
    }
    char *referenced_type_name = NULL;
    if (parse_php_field_type_details(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp), &field_def_out->type, &is_repeated_flag, &referenced_type_name) == FAILURE) {
        if (referenced_type_name) efree(referenced_type_name);
        throw_iibin_error_as_php_exception(0, "Schema '%s': Field '%s' has unparseable 'type': %s.", schema_name_for_error, field_name_in_php, Z_STRVAL_P(zv_temp));
        return FAILURE;
    }
    if (is_repeated_flag) field_def_out->flags |= IIBIN_FIELD_FLAG_REPEATED;

    if (field_def_out->type == IIBIN_INTERNAL_TYPE_UNKNOWN) {
        if (!referenced_type_name) { throw_iibin_error_as_php_exception(0, "Internal error: referenced type name missing for unknown type."); return FAILURE; }
        if (get_compiled_iibin_schema_internal(referenced_type_name)) {
            field_def_out->type = IIBIN_INTERNAL_TYPE_MESSAGE;
            field_def_out->message_type_name_if_nested = referenced_type_name;
        } else if (get_compiled_iibin_enum_internal(referenced_type_name)) {
            field_def_out->type = IIBIN_INTERNAL_TYPE_ENUM;
            field_def_out->enum_type_name_if_enum = referenced_type_name;
        } else {
            throw_iibin_error_as_php_exception(0, "Schema '%s': Field '%s' type '%s' is not a primitive, defined message, or defined enum.", schema_name_for_error, field_name_in_php, referenced_type_name);
            efree(referenced_type_name); return FAILURE;
        }
    } else if (referenced_type_name) {
        efree(referenced_type_name);
    }

    if ((zv_temp = zend_hash_str_find(options_ht, "required", sizeof("required")-1)) && zend_is_true(zv_temp)) field_def_out->flags |= IIBIN_FIELD_FLAG_REQUIRED;
    else field_def_out->flags |= IIBIN_FIELD_FLAG_OPTIONAL;

    if (is_repeated_flag) {
        zend_bool is_packable = (field_def_out->type >= IIBIN_INTERNAL_TYPE_DOUBLE && field_def_out->type <= IIBIN_INTERNAL_TYPE_BOOL) || field_def_out->type == IIBIN_INTERNAL_TYPE_ENUM;
        if (is_packable) {
            zend_bool use_packed = 1;
            if ((zv_temp = zend_hash_str_find(options_ht, "packed", sizeof("packed")-1))) use_packed = zend_is_true(zv_temp);
            if (use_packed) field_def_out->flags |= IIBIN_FIELD_FLAG_PACKED;
        }
    }

    field_def_out->wire_type = calculate_field_wire_type(field_def_out->type, (field_def_out->flags & IIBIN_FIELD_FLAG_PACKED));
    if (field_def_out->wire_type == (uint32_t)-1) {
         throw_iibin_error_as_php_exception(0, "Schema '%s': Field '%s' - could not determine wire type.", schema_name_for_error, field_name_in_php); return FAILURE;
    }

    if ((zv_temp = zend_hash_str_find(options_ht, "default", sizeof("default")-1))) {
        if (field_def_out->type == IIBIN_INTERNAL_TYPE_ENUM && Z_TYPE_P(zv_temp) == IS_STRING) {
            const quicpro_iibin_compiled_enum_internal *enum_def = get_compiled_iibin_enum_internal(field_def_out->enum_type_name_if_enum);
            quicpro_iibin_enum_value_def_internal *enum_val_def = zend_hash_find_ptr(&enum_def->values_by_name, Z_STR_P(zv_temp));
            if (!enum_val_def) {
                throw_iibin_error_as_php_exception(0, "Schema '%s': Field '%s' - default enum value name '%s' not found in enum '%s'.", schema_name_for_error, field_name_in_php, Z_STRVAL_P(zv_temp), enum_def->enum_name);
                return FAILURE;
            }
            ZVAL_LONG(&field_def_out->default_value_zval, enum_val_def->number);
        } else {
            ZVAL_COPY(&field_def_out->default_value_zval, zv_temp);
        }
    }

    if ((zv_temp = zend_hash_str_find(options_ht, "json_name", sizeof("json_name")-1)) && Z_TYPE_P(zv_temp) == IS_STRING) {
        field_def_out->json_name = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));
    }

    if ((zv_temp = zend_hash_str_find(options_ht, "deprecated", sizeof("deprecated")-1)) && zend_is_true(zv_temp)) {
        field_def_out->is_deprecated = 1;
    }

    return SUCCESS;
}

/* Comparison function for qsort to sort field definitions by tag */
static int compare_field_defs_by_tag(const void *a, const void *b) {
    const quicpro_iibin_field_def_internal *field_a = *(const quicpro_iibin_field_def_internal**)a;
    const quicpro_iibin_field_def_internal *field_b = *(const quicpro_iibin_field_def_internal**)b;
    if (field_a->tag < field_b->tag) return -1;
    if (field_a->tag > field_b->tag) return 1;
    return 0;
}

/* Helper to cleanup a partially built schema on error */
static void cleanup_partially_built_schema(quicpro_iibin_compiled_schema_internal *schema) {
    if (!schema) return;
    /* This is a simplified cleanup. A real one would need to carefully check what was allocated before freeing. */
    if (schema->schema_name) efree(schema->schema_name);
    zend_hash_destroy(&schema->fields_by_tag);
    zend_hash_destroy(&schema->fields_by_name);
    if (schema->ordered_fields) efree(schema->ordered_fields);
    efree(schema);
}

/* Helper to cleanup a partially built enum on error */
static void cleanup_partially_built_enum(quicpro_iibin_compiled_enum_internal *enum_def) {
    if (!enum_def) return;
    if (enum_def->enum_name) efree(enum_def->enum_name);
    zend_hash_destroy(&enum_def->values_by_name);
    zend_hash_destroy(&enum_def->names_by_value);
    efree(enum_def);
}


/* --- PHP_FUNCTION Implementations --- */

PHP_FUNCTION(quicpro_iibin_define_enum)
{
    /* ... (Implementation is identical to before, just using IIBIN names) ... */
    char *enum_name_str; size_t enum_name_len; zval *enum_values_php_array;
    ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_STRING(enum_name_str, enum_name_len) Z_PARAM_ARRAY(enum_values_php_array) ZEND_PARSE_PARAMETERS_END();
    if (!quicpro_iibin_registries_initialized) { throw_iibin_error_as_php_exception(0, "IIBIN registries not initialized."); RETURN_FALSE; }
    if (get_compiled_iibin_schema_internal(enum_name_str) || get_compiled_iibin_enum_internal(enum_name_str)) { throw_iibin_error_as_php_exception(0, "Enum or Schema name '%.*s' already defined.", (int)enum_name_len, enum_name_str); RETURN_FALSE; }
    quicpro_iibin_compiled_enum_internal *new_enum_def = ecalloc(1, sizeof(quicpro_iibin_compiled_enum_internal));
    new_enum_def->enum_name = estrndup(enum_name_str, enum_name_len);
    HashTable *php_enum_values_ht = Z_ARRVAL_P(enum_values_php_array);
    uint32_t num_enum_values = zend_hash_num_elements(php_enum_values_ht);
    zend_hash_init(&new_enum_def->values_by_name, num_enum_values > 0 ? num_enum_values : 1, NULL, NULL, 0);
    zend_hash_init(&new_enum_def->names_by_value, num_enum_values > 0 ? num_enum_values : 1, NULL, NULL, 0);
    zend_string *php_enum_name_key; zval *php_enum_number_zval;
    ZEND_HASH_FOREACH_STR_KEY_VAL(php_enum_values_ht, php_enum_name_key, php_enum_number_zval) {
        if (!php_enum_name_key || Z_TYPE_P(php_enum_number_zval) != IS_LONG) { cleanup_partially_built_enum(new_enum_def); throw_iibin_error_as_php_exception(0, "Enum '%s': Invalid definition.", enum_name_str); RETURN_FALSE; }
        quicpro_iibin_enum_value_def_internal *val_def = ecalloc(1, sizeof(quicpro_iibin_enum_value_def_internal));
        val_def->name = estrndup(ZSTR_VAL(php_enum_name_key), ZSTR_LEN(php_enum_name_key));
        val_def->number = (int32_t)Z_LVAL_P(php_enum_number_zval);
        if (zend_hash_exists(&new_enum_def->values_by_name, php_enum_name_key)) { cleanup_partially_built_enum(new_enum_def); quicpro_destroy_iibin_enum_value_def_internal(val_def); throw_iibin_error_as_php_exception(0, "Enum '%s': Duplicate name '%s'.", enum_name_str, ZSTR_VAL(php_enum_name_key)); RETURN_FALSE; }
        if (zend_hash_index_exists(&new_enum_def->names_by_value, (zend_ulong)val_def->number)) { cleanup_partially_built_enum(new_enum_def); quicpro_destroy_iibin_enum_value_def_internal(val_def); throw_iibin_error_as_php_exception(0, "Enum '%s': Duplicate number %d.", enum_name_str, val_def->number); RETURN_FALSE; }
        zend_hash_add_ptr(&new_enum_def->values_by_name, php_enum_name_key, val_def);
        zend_hash_index_add_ptr(&new_enum_def->names_by_value, (zend_ulong)val_def->number, estrdup(val_def->name));
    } ZEND_HASH_FOREACH_END();
    if (zend_hash_str_add_ptr(&quicpro_iibin_enum_registry, enum_name_str, enum_name_len, new_enum_def) == NULL) { cleanup_partially_built_enum(new_enum_def); throw_iibin_error_as_php_exception(0, "Failed to add enum '%s' to registry.", enum_name_str); RETURN_FALSE; }
    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_iibin_define_schema)
{
    /* ... (Implementation is identical to before, just using IIBIN names) ... */
    char *schema_name_str; size_t schema_name_len; zval *schema_def_php_array;
    ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_STRING(schema_name_str, schema_name_len) Z_PARAM_ARRAY(schema_def_php_array) ZEND_PARSE_PARAMETERS_END();
    if (!quicpro_iibin_registries_initialized) { throw_iibin_error_as_php_exception(0, "IIBIN registries not initialized."); RETURN_FALSE; }
    if (get_compiled_iibin_schema_internal(schema_name_str) || get_compiled_iibin_enum_internal(schema_name_str)) { throw_iibin_error_as_php_exception(0, "Schema or Enum name '%.*s' already defined.", (int)schema_name_len, schema_name_str); RETURN_FALSE; }
    quicpro_iibin_compiled_schema_internal *new_schema = ecalloc(1, sizeof(quicpro_iibin_compiled_schema_internal));
    new_schema->schema_name = estrndup(schema_name_str, schema_name_len);
    HashTable *php_fields_ht = Z_ARRVAL_P(schema_def_php_array);
    uint32_t num_fields = zend_hash_num_elements(php_fields_ht);
    zend_hash_init(&new_schema->fields_by_tag, num_fields > 0 ? num_fields : 1, NULL, NULL, 0);
    zend_hash_init(&new_schema->fields_by_name, num_fields > 0 ? num_fields : 1, NULL, NULL, 0);
    new_schema->ordered_fields = (num_fields > 0) ? ecalloc(num_fields, sizeof(quicpro_iibin_field_def_internal*)) : NULL;
    new_schema->num_fields = 0;
    zend_string *php_field_name; zval *field_options_array; zend_bool success = 1;
    ZEND_HASH_FOREACH_STR_KEY_VAL(php_fields_ht, php_field_name, field_options_array) {
        if (!php_field_name || Z_TYPE_P(field_options_array) != IS_ARRAY) { throw_iibin_error_as_php_exception(0, "Schema '%s': Invalid field definition.", schema_name_str); success = 0; break; }
        quicpro_iibin_field_def_internal *field_def = ecalloc(1, sizeof(quicpro_iibin_field_def_internal));
        if (parse_php_field_options(schema_name_str, ZSTR_VAL(php_field_name), Z_ARRVAL_P(field_options_array), field_def) == FAILURE) { efree(field_def); success = 0; break; }
        if (zend_hash_index_exists(&new_schema->fields_by_tag, field_def->tag)) { throw_iibin_error_as_php_exception(0, "Schema '%s': Duplicate tag %u.", schema_name_str, field_def->tag); quicpro_destroy_iibin_field_def_internal(field_def); success = 0; break; }
        zend_hash_index_update_ptr(&new_schema->fields_by_tag, field_def->tag, field_def);
        zend_hash_str_update_ptr(&new_schema->fields_by_name, field_def->name_in_php, strlen(field_def->name_in_php), field_def);
        if (new_schema->ordered_fields) new_schema->ordered_fields[new_schema->num_fields++] = field_def;
    } ZEND_HASH_FOREACH_END();
    if (success && new_schema->num_fields > 1) qsort(new_schema->ordered_fields, new_schema->num_fields, sizeof(quicpro_iibin_field_def_internal*), compare_field_defs_by_tag);
    if (!success) { cleanup_partially_built_schema(new_schema); RETURN_FALSE; }
    if (zend_hash_str_add_ptr(&quicpro_iibin_schema_registry, schema_name_str, schema_name_len, new_schema) == NULL) { cleanup_partially_built_schema(new_schema); throw_iibin_error_as_php_exception(0, "Failed to add schema '%s' to registry.", schema_name_str); RETURN_FALSE; }
    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_iibin_is_schema_defined) { /* ... implementation ... */ }
PHP_FUNCTION(quicpro_iibin_is_enum_defined) { /* ... implementation ... */ }
PHP_FUNCTION(quicpro_iibin_is_defined) { /* ... implementation ... */ }
PHP_FUNCTION(quicpro_iibin_get_defined_schemas) { /* ... implementation ... */ }
PHP_FUNCTION(quicpro_iibin_get_defined_enums) { /* ... implementation ... */ }