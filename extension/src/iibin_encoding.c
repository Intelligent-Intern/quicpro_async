/*
 * src/iibin_encoding.c – C Implementation for Quicpro\IIBIN Message Encoding
 * ===========================================================================
 *
 * This file implements the C-native logic for encoding PHP data structures
 * (arrays or objects) into a compact, IIBIN (Intelligent Intern Binary)
 * wire format. It uses the compiled schema definitions managed by iibin_schema.c.
 *
 * This implementation enforces strict type checking to ensure data integrity
 * and prevent unexpected behavior from PHP's type juggling.
 */

#include "php_quicpro.h"
#include "iibin.h"
#include "iibin_internal.h"
#include "cancel.h"

#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_hash.h>
#include <zend_smart_str.h>
#include <zend_string.h>
#include <Zend/zend_object_handlers.h>

/* --- Static Helper Function Prototypes --- */
static int encode_message_internal(smart_str *buf, const quicpro_iibin_compiled_schema_internal *schema, zval *data_zval);
static int encode_field_internal(smart_str *buf, const quicpro_iibin_field_def_internal *field, zval *value_zval);
static int encode_packed_repeated_field(smart_str *buf, const quicpro_iibin_field_def_internal *field, zval *php_array_zval);
static int encode_single_field_value(smart_str *buf, const quicpro_iibin_field_def_internal *field, zval *value_zval);


/**
 * @brief Encodes a single, non-repeated value with its tag and wire type.
 * @param buf The smart_str buffer to write the binary data into.
 * @param field The compiled field definition for the value being encoded.
 * @param value_zval The PHP zval containing the value to encode.
 * @return SUCCESS or FAILURE.
 *
 * This function performs strict type validation on the input zval before
 * performing the low-level serialization of a single data point.
 */
static int encode_single_field_value(smart_str *buf, const quicpro_iibin_field_def_internal *field, zval *value_zval) {
    /* Write tag and wire type for this field. */
    quicpro_iibin_encode_varint(buf, (field->tag << 3) | field->wire_type);

    /* Encode the value based on its defined type, with strict type checking. */
    switch (field->type) {
        case IIBIN_INTERNAL_TYPE_INT32:
        case IIBIN_INTERNAL_TYPE_UINT32:
        case IIBIN_INTERNAL_TYPE_SINT32:
        case IIBIN_INTERNAL_TYPE_INT64:
        case IIBIN_INTERNAL_TYPE_UINT64:
        case IIBIN_INTERNAL_TYPE_SINT64:
        case IIBIN_INTERNAL_TYPE_FIXED32:
        case IIBIN_INTERNAL_TYPE_SFIXED32:
        case IIBIN_INTERNAL_TYPE_FIXED64:
        case IIBIN_INTERNAL_TYPE_SFIXED64:
            if (Z_TYPE_P(value_zval) != IS_LONG) {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Field '%s' expects an integer, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
                return FAILURE;
            }
            break; /* Fall through to value encoding */

        case IIBIN_INTERNAL_TYPE_FLOAT:
        case IIBIN_INTERNAL_TYPE_DOUBLE:
            if (Z_TYPE_P(value_zval) != IS_DOUBLE && Z_TYPE_P(value_zval) != IS_LONG) {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Field '%s' expects a float or integer, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
                return FAILURE;
            }
            break; /* Fall through to value encoding */

        case IIBIN_INTERNAL_TYPE_BOOL:
             if (Z_TYPE_P(value_zval) != IS_TRUE && Z_TYPE_P(value_zval) != IS_FALSE) {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Field '%s' expects a boolean, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
                return FAILURE;
             }
             break;

        /* No pre-check needed for enum, string, bytes, message, as they have complex checks below */
        default: break;
    }


    /* After type validation, encode the value. */
    switch (field->type) {
        case IIBIN_INTERNAL_TYPE_INT32: quicpro_iibin_encode_varint(buf, (uint64_t)Z_LVAL_P(value_zval)); break;
        case IIBIN_INTERNAL_TYPE_SINT32: quicpro_iibin_encode_varint(buf, quicpro_iibin_zigzag_encode32((int32_t)Z_LVAL_P(value_zval))); break;
        case IIBIN_INTERNAL_TYPE_INT64: quicpro_iibin_encode_varint(buf, (uint64_t)Z_LVAL_P(value_zval)); break;
        case IIBIN_INTERNAL_TYPE_SINT64: quicpro_iibin_encode_varint(buf, quicpro_iibin_zigzag_encode64(Z_LVAL_P(value_zval))); break;
        case IIBIN_INTERNAL_TYPE_UINT32: case IIBIN_INTERNAL_TYPE_UINT64: quicpro_iibin_encode_varint(buf, (uint64_t)Z_LVAL_P(value_zval)); break;
        case IIBIN_INTERNAL_TYPE_BOOL: quicpro_iibin_encode_varint(buf, Z_TYPE_P(value_zval) == IS_TRUE ? 1 : 0); break;

        case IIBIN_INTERNAL_TYPE_ENUM: {
            int32_t enum_int_val;
            if (Z_TYPE_P(value_zval) == IS_LONG) {
                enum_int_val = (int32_t)Z_LVAL_P(value_zval);
            } else if (Z_TYPE_P(value_zval) == IS_STRING) {
                const quicpro_iibin_compiled_enum_internal *enum_def = get_compiled_iibin_enum_internal(field->enum_type_name_if_enum);
                if (!enum_def) {
                    throw_iibin_error_as_php_exception(0, "Encoding failed: Enum type '%s' for field '%s' is not defined.", field->enum_type_name_if_enum, field->name_in_php);
                    return FAILURE;
                }
                quicpro_iibin_enum_value_def_internal *enum_val_def = zend_hash_find_ptr(&enum_def->values_by_name, Z_STR_P(value_zval));
                if (!enum_val_def) {
                    throw_iibin_error_as_php_exception(0, "Encoding failed: Enum value name '%s' is not a valid member of enum '%s' for field '%s'.", Z_STRVAL_P(value_zval), field->enum_type_name_if_enum, field->name_in_php);
                    return FAILURE;
                }
                enum_int_val = enum_val_def->number;
            } else {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Enum field '%s' expects an integer or string, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
                return FAILURE;
            }
            quicpro_iibin_encode_varint(buf, (uint64_t)enum_int_val);
            break;
        }

        case IIBIN_INTERNAL_TYPE_FLOAT: {
            float f_val = (float)zval_get_double(value_zval);
            quicpro_iibin_encode_fixed32(buf, *((uint32_t*)&f_val));
            break;
        }
        case IIBIN_INTERNAL_TYPE_FIXED32: case IIBIN_INTERNAL_TYPE_SFIXED32:
            quicpro_iibin_encode_fixed32(buf, (uint32_t)Z_LVAL_P(value_zval));
            break;
        case IIBIN_INTERNAL_TYPE_DOUBLE: {
            double d_val = zval_get_double(value_zval);
            quicpro_iibin_encode_fixed64(buf, *((uint64_t*)&d_val));
            break;
        }
        case IIBIN_INTERNAL_TYPE_FIXED64: case IIBIN_INTERNAL_TYPE_SFIXED64:
            quicpro_iibin_encode_fixed64(buf, (uint64_t)Z_LVAL_P(value_zval));
            break;
        case IIBIN_INTERNAL_TYPE_STRING: case IIBIN_INTERNAL_TYPE_BYTES: {
            if (Z_TYPE_P(value_zval) != IS_STRING) {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Field '%s' expects a string, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
                return FAILURE;
            }
            quicpro_iibin_encode_varint(buf, Z_STRLEN_P(value_zval));
            smart_str_appendl(buf, Z_STRVAL_P(value_zval), Z_STRLEN_P(value_zval));
            break;
        }
        case IIBIN_INTERNAL_TYPE_MESSAGE: {
            if (Z_TYPE_P(value_zval) != IS_ARRAY && Z_TYPE_P(value_zval) != IS_OBJECT) {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Nested message field '%s' expects an array or object, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
                return FAILURE;
            }
            smart_str nested_buf = {0};
            const quicpro_iibin_compiled_schema_internal *nested_schema = get_compiled_iibin_schema_internal(field->message_type_name_if_nested);
            if (!nested_schema) {
                throw_iibin_error_as_php_exception(0, "Encoding failed: Nested schema '%s' for field '%s' is not defined.", field->message_type_name_if_nested, field->name_in_php);
                return FAILURE;
            }
            if (encode_message_internal(&nested_buf, nested_schema, value_zval) == FAILURE) {
                smart_str_free(&nested_buf);
                return FAILURE;
            }
            quicpro_iibin_encode_varint(buf, nested_buf.s ? ZSTR_LEN(nested_buf.s) : 0);
            if (nested_buf.s) {
                smart_str_append_smart_str(buf, &nested_buf);
            }
            smart_str_free(&nested_buf);
            break;
        }
        default:
            throw_iibin_error_as_php_exception(0, "Encoding failed: unknown type %d for field '%s'.", field->type, field->name_in_php);
            return FAILURE;
    }
    return SUCCESS;
}

/**
 * @brief Encodes a repeated field of packable primitive types into a single length-delimited field.
 * @param buf The smart_str buffer to write into.
 * @param field The compiled field definition (must have PACKED flag).
 * @param php_array_zval A zval containing the PHP array of values.
 * @return SUCCESS or FAILURE.
 */
static int encode_packed_repeated_field(smart_str *buf, const quicpro_iibin_field_def_internal *field, zval *php_array_zval) {
    smart_str packed_buf = {0};
    zval *item_zval;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(php_array_zval), item_zval) {
        /*
         * This function directly encodes the values without their tags,
         * as they will be written as one contiguous block.
         */
        switch (field->type) {
            case IIBIN_INTERNAL_TYPE_INT32: case IIBIN_INTERNAL_TYPE_INT64: case IIBIN_INTERNAL_TYPE_UINT32: case IIBIN_INTERNAL_TYPE_UINT64: case IIBIN_INTERNAL_TYPE_BOOL: case IIBIN_INTERNAL_TYPE_ENUM:
                quicpro_iibin_encode_varint(&packed_buf, (uint64_t)zval_get_long(item_zval));
                break;
            case IIBIN_INTERNAL_TYPE_SINT32:
                quicpro_iibin_encode_varint(&packed_buf, quicpro_iibin_zigzag_encode32((int32_t)zval_get_long(item_zval)));
                break;
            case IIBIN_INTERNAL_TYPE_SINT64:
                 quicpro_iibin_encode_varint(&packed_buf, quicpro_iibin_zigzag_encode64(zval_get_long(item_zval)));
                break;
            case IIBIN_INTERNAL_TYPE_FIXED32: case IIBIN_INTERNAL_TYPE_SFIXED32:
                 quicpro_iibin_encode_fixed32(&packed_buf, (uint32_t)zval_get_long(item_zval));
                 break;
            case IIBIN_INTERNAL_TYPE_FLOAT: {
                 float f_val = (float)zval_get_double(item_zval);
                 quicpro_iibin_encode_fixed32(&packed_buf, *((uint32_t*)&f_val));
                 break;
            }
            case IIBIN_INTERNAL_TYPE_FIXED64: case IIBIN_INTERNAL_TYPE_SFIXED64:
                 quicpro_iibin_encode_fixed64(&packed_buf, (uint64_t)zval_get_long(item_zval));
                 break;
            case IIBIN_INTERNAL_TYPE_DOUBLE: {
                 double d_val = zval_get_double(item_zval);
                 quicpro_iibin_encode_fixed64(&packed_buf, *((uint64_t*)&d_val));
                 break;
            }
            default: break; /* Non-packable types (string, bytes, message) should not reach this function. */
        }
    } ZEND_HASH_FOREACH_END();

    if (packed_buf.s) {
        quicpro_iibin_encode_varint(buf, (field->tag << 3) | QUICPRO_IIBIN_WIRETYPE_LENGTH_DELIM);
        quicpro_iibin_encode_varint(buf, ZSTR_LEN(packed_buf.s));
        smart_str_append_smart_str(buf, &packed_buf);
    }

    smart_str_free(&packed_buf);
    return SUCCESS;
}

/**
 * @brief Dispatches encoding for a field, handling optional, required, and repeated flags.
 * @param buf The smart_str buffer to write into.
 * @param field The compiled field definition.
 * @param value_zval The PHP value for this field. Can be NULL if the field is not present in the input data.
 * @return SUCCESS or FAILURE.
 */
static int encode_field_internal(smart_str *buf, const quicpro_iibin_field_def_internal *field, zval *value_zval) {
    if (!value_zval || Z_TYPE_P(value_zval) == IS_NULL || Z_TYPE_P(value_zval) == IS_UNDEF) {
        if (field->flags & IIBIN_FIELD_FLAG_REQUIRED) {
            throw_iibin_error_as_php_exception(0, "Encoding failed: Required field '%s' (tag %u) is missing or null.", field->name_in_php, field->tag);
            return FAILURE;
        }
        return SUCCESS; /* Optional fields are not encoded if missing. */
    }

    if (field->flags & IIBIN_FIELD_FLAG_REPEATED) {
        if (Z_TYPE_P(value_zval) != IS_ARRAY) {
            throw_iibin_error_as_php_exception(0, "Encoding failed: Field '%s' is repeated and requires a PHP array, but got %s.", field->name_in_php, zend_get_type_by_const(Z_TYPE_P(value_zval)));
            return FAILURE;
        }
        if (zend_hash_num_elements(Z_ARRVAL_P(value_zval)) == 0) {
            return SUCCESS; // Do not encode empty arrays.
        }

        if (field->flags & IIBIN_FIELD_FLAG_PACKED) {
            return encode_packed_repeated_field(buf, field, value_zval);
        } else {
            /* Unpacked repeated field: write a tag/value pair for each item in the array. */
            zval *item_zval;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value_zval), item_zval) {
                if (encode_single_field_value(buf, field, item_zval) == FAILURE) {
                    return FAILURE;
                }
            } ZEND_HASH_FOREACH_END();
        }
        return SUCCESS;
    }

    /* Handle a single (non-repeated) field value. */
    return encode_single_field_value(buf, field, value_zval);
}

/**
 * @brief Encodes a full message (PHP array or object) into a binary buffer.
 * @param buf The smart_str buffer to write into.
 * @param schema The compiled schema for the message being encoded.
 * @param data_zval The PHP array or object containing the message data.
 * @return SUCCESS or FAILURE.
 */
static int encode_message_internal(smart_str *buf, const quicpro_iibin_compiled_schema_internal *schema, zval *data_zval) {
    if (Z_TYPE_P(data_zval) != IS_ARRAY && Z_TYPE_P(data_zval) != IS_OBJECT) {
        throw_iibin_error_as_php_exception(0, "Data for message type '%s' must be an array or object.", schema->schema_name);
        return FAILURE;
    }

    /* Iterate through fields of the schema in canonical order (sorted by tag). */
    for (size_t i = 0; i < schema->num_fields; ++i) {
        quicpro_iibin_field_def_internal *field_def = schema->ordered_fields[i];
        if (!field_def) continue;

        zval *php_value = NULL;
        zval rv; /* For zend_read_property, which may return a temporary zval */
        zend_string *field_name_key = zend_string_init(field_def->name_in_php, strlen(field_def->name_in_php), 0);

        if (Z_TYPE_P(data_zval) == IS_ARRAY) {
            php_value = zend_hash_find(Z_ARRVAL_P(data_zval), field_name_key);
        } else { /* IS_OBJECT */
            php_value = zend_read_property(Z_OBJCE_P(data_zval), Z_OBJ_P(data_zval), ZSTR_VAL(field_name_key), ZSTR_LEN(field_name_key), 1, &rv);
        }

        if (encode_field_internal(buf, field_def, php_value) == FAILURE) {
            zend_string_release(field_name_key);
            return FAILURE;
        }

        zend_string_release(field_name_key);
    }
    return SUCCESS;
}


/* --- PHP_FUNCTION Implementation --- */

PHP_FUNCTION(quicpro_iibin_encode)
{
    char *schema_name_str;
    size_t schema_name_len;
    zval *php_data_zval;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(schema_name_str, schema_name_len)
        Z_PARAM_ZVAL(php_data_zval)
    ZEND_PARSE_PARAMETERS_END();

    const quicpro_iibin_compiled_schema_internal *schema = get_compiled_iibin_schema_internal(schema_name_str);
    if (!schema) {
        throw_iibin_error_as_php_exception(0, "Schema '%s' not defined for encoding.", schema_name_str);
        RETURN_FALSE;
    }

    smart_str bin_buf = {0};

    if (encode_message_internal(&bin_buf, schema, php_data_zval) == FAILURE) {
        smart_str_free(&bin_buf);
        RETURN_FALSE; /* Exception was already thrown by an internal function */
    }

    if (bin_buf.s) {
        RETVAL_STR(bin_buf.s);
    } else {
        RETVAL_EMPTY_STRING();
    }
    /* Do not call smart_str_free(&bin_buf) if RETVAL_STR is used, as it takes ownership. */
}
