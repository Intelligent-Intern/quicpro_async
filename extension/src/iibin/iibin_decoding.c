/*
 * src/iibin_decoding.c – C Implementation for Quicpro\IIBIN Message Decoding
 * ===========================================================================
 *
 * This file implements the C-native logic for decoding a compact,
 * IIBIN (Intelligent Intern Binary) wire format string back into a PHP data structure
 * (associative array or stdClass object).
 * It uses the compiled schema definitions managed by iibin_schema.c.
 */

#include "php_quicpro.h"
#include "iibin.h"
#include "iibin_internal.h"
#include "cancel.h"

#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_hash.h>
#include <zend_string.h>
#include <Zend/zend_object_handlers.h>

/* --- Static Helper Function Prototypes --- */
static int decode_message_internal(const unsigned char **buf_ptr, const unsigned char *buf_end, const quicpro_iibin_compiled_schema_internal *schema, zval *return_zval, zend_bool decode_as_object);
static int populate_default_values_and_check_required(const quicpro_iibin_compiled_schema_internal *schema, zval *decoded_message_zval);
static zend_bool skip_field(const unsigned char **buf_ptr, const unsigned char *buf_end, uint32_t wire_type);
static int decode_value_to_zval(const unsigned char **buf_ptr, const unsigned char *buf_end, const quicpro_iibin_field_def_internal *field, uint32_t wire_type, zval *out_zval, zend_bool decode_as_object);


/**
 * @brief Decodes a single value from the buffer based on its wire type and schema definition.
 * @param buf_ptr Pointer to a pointer to the current position in the read buffer. Will be advanced.
 * @param buf_end Pointer to the end of the read buffer (for boundary checks).
 * @param field Pointer to the compiled field definition from the schema.
 * @param wire_type The wire type read from the buffer for this field.
 * @param out_zval A pointer to a zval that will be populated with the decoded PHP value.
 * @param decode_as_object If true, nested messages will be decoded as stdClass objects.
 * @return SUCCESS or FAILURE.
 *
 * This function handles the low-level conversion from binary wire format to a PHP zval.
 */
static int decode_value_to_zval(
    const unsigned char **buf_ptr, const unsigned char *buf_end,
    const quicpro_iibin_field_def_internal *field,
    uint32_t wire_type, zval *out_zval, zend_bool decode_as_object
) {
    uint64_t u64_val;
    uint32_t u32_val;

    /*
     * For unpacked repeated fields, the wire type in the buffer must match the schema's expected wire type.
     * For packed repeated fields, this check is bypassed because the containing field is LENGTH_DELIM,
     * but the individual items inside have their own primitive wire type. The caller handles this.
     */
    if (wire_type != field->wire_type && !(field->flags & IIBIN_FIELD_FLAG_PACKED)) {
        throw_iibin_error_as_php_exception(0, "Schema '%s': Wire type mismatch for field '%s' (tag %u). Expected wire type %u, but got %u on the wire.",
            field->message_type_name_if_nested ? field->message_type_name_if_nested : (field->enum_type_name_if_enum ? field->enum_type_name_if_enum : "Primitive"),
            field->name_in_php, field->tag, field->wire_type, wire_type);
        return FAILURE;
    }

    switch (field->type) {
        case IIBIN_INTERNAL_TYPE_INT32:
        case IIBIN_INTERNAL_TYPE_ENUM:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_LONG(out_zval, (int32_t)u64_val);
            break;
        case IIBIN_INTERNAL_TYPE_SINT32:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_LONG(out_zval, quicpro_iibin_zigzag_decode32((uint32_t)u64_val));
            break;
        case IIBIN_INTERNAL_TYPE_INT64:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_LONG(out_zval, (int64_t)u64_val);
            break;
        case IIBIN_INTERNAL_TYPE_SINT64:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_LONG(out_zval, quicpro_iibin_zigzag_decode64(u64_val));
            break;
        case IIBIN_INTERNAL_TYPE_UINT32:
        case IIBIN_INTERNAL_TYPE_UINT64:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_LONG(out_zval, (zend_long)u64_val); /* Note: may truncate on 32-bit PHP for large uint64 */
            break;
        case IIBIN_INTERNAL_TYPE_BOOL:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_BOOL(out_zval, u64_val != 0);
            break;

        case IIBIN_INTERNAL_TYPE_FLOAT:
            if (!quicpro_iibin_decode_fixed32(buf_ptr, buf_end, &u32_val)) return FAILURE;
            ZVAL_DOUBLE(out_zval, (double)(*((float*)&u32_val)));
            break;
        case IIBIN_INTERNAL_TYPE_FIXED32:
        case IIBIN_INTERNAL_TYPE_SFIXED32:
            if (!quicpro_iibin_decode_fixed32(buf_ptr, buf_end, &u32_val)) return FAILURE;
            ZVAL_LONG(out_zval, (field->type == IIBIN_INTERNAL_TYPE_FIXED32) ? (zend_long)u32_val : (int32_t)u32_val);
            break;
        case IIBIN_INTERNAL_TYPE_DOUBLE:
            if (!quicpro_iibin_decode_fixed64(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_DOUBLE(out_zval, *((double*)&u64_val));
            break;
        case IIBIN_INTERNAL_TYPE_FIXED64:
        case IIBIN_INTERNAL_TYPE_SFIXED64:
            if (!quicpro_iibin_decode_fixed64(buf_ptr, buf_end, &u64_val)) return FAILURE;
            ZVAL_LONG(out_zval, (zend_long)u64_val); /* Note: may truncate on 32-bit PHP */
            break;

        case IIBIN_INTERNAL_TYPE_STRING:
        case IIBIN_INTERNAL_TYPE_BYTES: {
            uint64_t len;
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &len)) return FAILURE;
            if (*buf_ptr + len > *buf_end) return FAILURE;
            ZVAL_STRINGL(out_zval, (char*)*buf_ptr, len);
            *buf_ptr += len;
            break;
        }
        case IIBIN_INTERNAL_TYPE_MESSAGE: {
            uint64_t len;
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &len)) return FAILURE;
            if (*buf_ptr + len > *buf_end) return FAILURE;

            const unsigned char *nested_buf_end = *buf_ptr + len;
            const quicpro_iibin_compiled_schema_internal *nested_schema = get_compiled_iibin_schema_internal(field->message_type_name_if_nested);
            if (!nested_schema) {
                throw_iibin_error_as_php_exception(0, "Decoding error: schema '%s' for nested field '%s' not defined.", field->message_type_name_if_nested, field->name_in_php);
                return FAILURE;
            }

            if (decode_as_object) {
                object_init(out_zval);
            } else {
                array_init(out_zval);
            }

            const unsigned char *nested_buf_start = *buf_ptr;
            if (decode_message_internal(&nested_buf_start, nested_buf_end, nested_schema, out_zval, decode_as_object) == FAILURE) {
                zval_ptr_dtor(out_zval); /* Clean up partially created array/object */
                return FAILURE;
            }
            *buf_ptr = nested_buf_end; /* Advance parent buffer by the full length of the nested message */
            break;
        }
        default:
            return FAILURE;
    }
    return SUCCESS;
}

/**
 * @brief Skips over a field in the buffer that is not defined in the schema.
 * @param buf_ptr Pointer to a pointer to the current position in the read buffer. Will be advanced.
 * @param buf_end Pointer to the end of the read buffer (for boundary checks).
 * @param wire_type The wire type of the field to skip.
 * @return 1 on success, 0 on failure (e.g., buffer underflow).
 *
 * This function is crucial for forward compatibility, allowing new clients to talk to
 * old servers without breaking.
 */
static zend_bool skip_field(const unsigned char **buf_ptr, const unsigned char *buf_end, uint32_t wire_type) {
    uint64_t len;
    switch (wire_type) {
        case QUICPRO_IIBIN_WIRETYPE_VARINT:
            return quicpro_iibin_decode_varint(buf_ptr, buf_end, &len);
        case QUICPRO_IIBIN_WIRETYPE_FIXED64:
            if (*buf_ptr + 8 > buf_end) return 0;
            *buf_ptr += 8;
            return 1;
        case QUICPRO_IIBIN_WIRETYPE_LENGTH_DELIM:
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &len)) return 0;
            if (*buf_ptr + len > buf_end) return 0;
            *buf_ptr += len;
            return 1;
        case QUICPRO_IIBIN_WIRETYPE_FIXED32:
            if (*buf_ptr + 4 > buf_end) return 0;
            *buf_ptr += 4;
            return 1;
        default:
            return 0; /* Unknown wire type, cannot skip safely */
    }
}

/**
 * @brief Recursively decodes a full message from the buffer into a PHP zval.
 * @param buf_ptr Pointer to a pointer to the current position in the read buffer.
 * @param buf_end Pointer to the end of the read buffer for the current message.
 * @param schema The compiled schema for the message being decoded.
 * @param return_zval The PHP zval (array or object) to populate.
 * @param decode_as_object Flag to indicate if nested messages should be objects.
 * @return SUCCESS or FAILURE.
 *
 * This is the core decoding loop. It reads field tags, looks up field definitions,
 * and dispatches to value decoders or handles repeated/packed fields.
 */
static int decode_message_internal(const unsigned char **buf_ptr, const unsigned char *buf_end, const quicpro_iibin_compiled_schema_internal *schema, zval *return_zval, zend_bool decode_as_object) {
    while (*buf_ptr < buf_end) {
        uint64_t key;
        if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &key)) {
            throw_iibin_error_as_php_exception(0, "Decoding error: malformed tag/wire_type varint in schema '%s'.", schema->schema_name);
            return FAILURE;
        }

        uint32_t tag = (uint32_t)(key >> 3);
        uint32_t wire_type = (uint32_t)(key & 0x7);
        if (tag == 0) continue;

        quicpro_iibin_field_def_internal *field = zend_hash_index_find_ptr(&schema->fields_by_tag, tag);

        if (!field) {
            if (!skip_field(buf_ptr, buf_end, wire_type)) {
                throw_iibin_error_as_php_exception(0, "Decoding error: failed to skip unknown field with tag %u in schema '%s'.", tag, schema->schema_name);
                return FAILURE;
            }
            continue;
        }

        zval value_zval;
        ZVAL_UNDEF(&value_zval);

        if ((field->flags & IIBIN_FIELD_FLAG_PACKED) && wire_type == QUICPRO_IIBIN_WIRETYPE_LENGTH_DELIM) {
            /* Packed repeated field: read length, then loop decode items */
            uint64_t len;
            if (!quicpro_iibin_decode_varint(buf_ptr, buf_end, &len)) return FAILURE;
            if (*buf_ptr + len > buf_end) { throw_iibin_error_as_php_exception(0, "Decoding error: packed field length exceeds buffer size."); return FAILURE; }

            const unsigned char *packed_end = *buf_ptr + len;
            zval *sub_array = zend_hash_str_find(Z_ARRVAL_P(return_zval), field->name_in_php, strlen(field->name_in_php));
            if (!sub_array) {
                zval new_array;
                array_init(&new_array);
                sub_array = zend_hash_str_update(Z_ARRVAL_P(return_zval), field->name_in_php, strlen(field->name_in_php), &new_array);
            }

            while (*buf_ptr < packed_end) {
                zval item_val;
                if (decode_value_to_zval(buf_ptr, packed_end, field, field->wire_type, &item_val, decode_as_object) == FAILURE) return FAILURE;
                add_next_index_zval(sub_array, &item_val);
            }
        } else {
            /* Single value or unpacked repeated value */
            if (decode_value_to_zval(buf_ptr, buf_end, field, wire_type, &value_zval, decode_as_object) == FAILURE) return FAILURE;

            if (field->flags & IIBIN_FIELD_FLAG_REPEATED) {
                zval *sub_array = zend_hash_str_find(is_object ? Z_OBJPROP_P(return_zval) : Z_ARRVAL_P(return_zval), field->name_in_php, strlen(field->name_in_php));
                if (!sub_array) {
                    zval new_array;
                    array_init(&new_array);
                    add_next_index_zval(&new_array, &value_zval);
                    if (is_object) zend_update_property(Z_OBJCE_P(return_zval), Z_OBJ_P(return_zval), field->name_in_php, strlen(field->name_in_php), &new_array);
                    else zend_hash_str_update(Z_ARRVAL_P(return_zval), field->name_in_php, strlen(field->name_in_php), &new_array);
                    zval_ptr_dtor(&new_array);
                } else {
                    add_next_index_zval(sub_array, &value_zval);
                }
            } else {
                if (is_object) zend_update_property(Z_OBJCE_P(return_zval), Z_OBJ_P(return_zval), field->name_in_php, strlen(field->name_in_php), &value_zval);
                else add_assoc_zval_ex(return_zval, field->name_in_php, strlen(field->name_in_php), &value_zval);
                zval_ptr_dtor(&value_zval);
            }
        }
    }
    return SUCCESS;
}

/**
 * @brief Post-decoding step to populate defaults and check required fields.
 * @param schema The compiled schema for the message.
 * @param decoded_message_zval The PHP zval (array or object) to process.
 * @return SUCCESS or FAILURE.
 */
static int populate_default_values_and_check_required(const quicpro_iibin_compiled_schema_internal *schema, zval *decoded_message_zval) {
    zend_bool is_object = (Z_TYPE_P(decoded_message_zval) == IS_OBJECT);

    for (size_t i = 0; i < schema->num_fields; ++i) {
        quicpro_iibin_field_def_internal *field = schema->ordered_fields[i];
        zend_bool field_is_present = is_object ?
            zend_hash_str_exists(Z_OBJPROP_P(decoded_message_zval), field->name_in_php, strlen(field->name_in_php)) :
            zend_hash_str_exists(Z_ARRVAL_P(decoded_message_zval), field->name_in_php, strlen(field->name_in_php));

        if (!field_is_present) {
            if (field->flags & IIBIN_FIELD_FLAG_REQUIRED) {
                throw_iibin_error_as_php_exception(0, "Decoding error: Required field '%s' (tag %u) not found in payload for schema '%s'.", field->name_in_php, field->tag, schema->schema_name);
                return FAILURE;
            }
            if (Z_TYPE(field->default_value_zval) != IS_UNDEF) {
                if (is_object) {
                    zend_update_property(Z_OBJCE_P(decoded_message_zval), Z_OBJ_P(decoded_message_zval), field->name_in_php, strlen(field->name_in_php), &field->default_value_zval);
                } else {
                    zend_hash_str_update(Z_ARRVAL_P(decoded_message_zval), field->name_in_php, strlen(field->name_in_php), &field->default_value_zval);
                    /* zend_hash_str_update copies the zval, so we must increment refcount. */
                    Z_TRY_ADDREF(field->default_value_zval);
                }
            }
        }
    }
    return SUCCESS;
}


/* --- PHP_FUNCTION Implementation --- */

PHP_FUNCTION(quicpro_iibin_decode)
{
    char *schema_name_str;
    size_t schema_name_len;
    char *binary_data_str;
    size_t binary_data_len;
    zend_bool decode_as_object = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STRING(schema_name_str, schema_name_len)
        Z_PARAM_STRING(binary_data_str, binary_data_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(decode_as_object)
    ZEND_PARSE_PARAMETERS_END();

    const quicpro_iibin_compiled_schema_internal *schema = get_compiled_iibin_schema_internal(schema_name_str);
    if (!schema) {
        throw_iibin_error_as_php_exception(0, "Schema '%s' not defined for decoding.", schema_name_str);
        RETURN_FALSE;
    }

    if (decode_as_object) {
        object_init(return_value);
    } else {
        array_init(return_value);
    }

    const unsigned char *buf_ptr = (const unsigned char *)binary_data_str;
    const unsigned char *buf_end = buf_ptr + binary_data_len;

    if (decode_message_internal(&buf_ptr, buf_end, schema, return_value, decode_as_object) == FAILURE) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE; /* Exception already thrown */
    }

    if (buf_ptr != buf_end) {
        throw_iibin_error_as_php_exception(0, "Decoding warning: Not all bytes were consumed for schema '%s'. %zu bytes remain.", schema->schema_name, (size_t)(buf_end - buf_ptr));
    }

    if (populate_default_values_and_check_required(schema, return_value) == FAILURE) {
         zval_ptr_dtor(return_value);
         RETURN_FALSE; /* Exception already thrown */
    }
}
