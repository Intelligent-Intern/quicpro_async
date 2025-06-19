/*
 * src/proto_internal.h – Internal Definitions for Quicpro\Proto Module
 * =====================================================================
 *
 * This header file contains C struct definitions, constants, global variable
 * declarations, and static inline wire format utilities shared across the
 * different C source files that implement the Quicpro\Proto functionality.
 */

#ifndef QUICPRO_PROTO_INTERNAL_H
#define QUICPRO_PROTO_INTERNAL_H

#include <php.h>
#include <stdint.h>
#include <zend_hash.h>
#include <zend_smart_str.h>

/* --- Wire Format Constants (Protobuf-like) --- */
#define QUICPRO_WIRETYPE_VARINT         0
#define QUICPRO_WIRETYPE_FIXED64        1
#define QUICPRO_WIRETYPE_LENGTH_DELIM   2
#define QUICPRO_WIRETYPE_FIXED32        5

/* --- Internal Data Structures for Compiled Schemas & Enums --- */

typedef enum _quicpro_proto_field_type_internal {
    PROTO_INTERNAL_TYPE_UNKNOWN = 0,
    PROTO_INTERNAL_TYPE_DOUBLE,
    PROTO_INTERNAL_TYPE_FLOAT,
    PROTO_INTERNAL_TYPE_INT64,
    PROTO_INTERNAL_TYPE_UINT64,
    PROTO_INTERNAL_TYPE_INT32,
    PROTO_INTERNAL_TYPE_UINT32,
    PROTO_INTERNAL_TYPE_SINT32,
    PROTO_INTERNAL_TYPE_SINT64,
    PROTO_INTERNAL_TYPE_FIXED64,
    PROTO_INTERNAL_TYPE_SFIXED64,
    PROTO_INTERNAL_TYPE_FIXED32,
    PROTO_INTERNAL_TYPE_SFIXED32,
    PROTO_INTERNAL_TYPE_BOOL,
    PROTO_INTERNAL_TYPE_STRING,
    PROTO_INTERNAL_TYPE_BYTES,
    PROTO_INTERNAL_TYPE_MESSAGE,
    PROTO_INTERNAL_TYPE_ENUM
} quicpro_proto_field_type_internal;

#define PROTO_FIELD_FLAG_NONE     0x00
#define PROTO_FIELD_FLAG_OPTIONAL 0x01
#define PROTO_FIELD_FLAG_REQUIRED 0x02
#define PROTO_FIELD_FLAG_REPEATED 0x04
#define PROTO_FIELD_FLAG_PACKED   0x08

typedef struct _quicpro_proto_field_def_internal {
    char *name_in_php;
    uint32_t tag;
    quicpro_proto_field_type_internal type;
    uint8_t flags;
    char *json_name;
    zend_bool is_deprecated;
    char *message_type_name_if_nested;
    char *enum_type_name_if_enum;
    zval default_value_zval;
    uint32_t wire_type;
} quicpro_proto_field_def_internal;

typedef struct _quicpro_compiled_schema_internal {
    char *schema_name;
    HashTable fields_by_tag;
    HashTable fields_by_name;
    quicpro_proto_field_def_internal **ordered_fields;
    size_t num_fields;
} quicpro_compiled_schema_internal;

typedef struct _quicpro_proto_enum_value_def_internal {
    char *name;
    int32_t number;
} quicpro_proto_enum_value_def_internal;

typedef struct _quicpro_compiled_enum_internal {
    char *enum_name;
    HashTable values_by_name;
    HashTable names_by_value;
} quicpro_compiled_enum_internal;


/* --- Global Schema and Enum Registry Extern Declarations --- */
/* Defined in proto_schema.c */
extern HashTable quicpro_schema_registry;
extern HashTable quicpro_enum_registry;
extern zend_bool quicpro_registries_initialized;


/* --- Internal C Utility Function Prototypes --- */
/* (Defined in proto_schema.c) */

const quicpro_compiled_schema_internal* get_compiled_schema_internal(const char *schema_name);
const quicpro_compiled_enum_internal* get_compiled_enum_internal(const char *enum_name);


/* --- Static Inline Low-Level Wire Format Utilities --- */

static inline void quicpro_proto_encode_varint(smart_str *buf, uint64_t value) {
    unsigned char temp_buf[10];
    int i = 0;
    do {
        temp_buf[i] = (unsigned char)(value & 0x7FU);
        value >>= 7;
        if (value > 0) {
            temp_buf[i] |= 0x80U;
        }
        i++;
    } while (value > 0 && i < 10);
    smart_str_appendl(buf, (char*)temp_buf, i);
}

static inline zend_bool quicpro_proto_decode_varint(const unsigned char **buf_ptr, const unsigned char *buf_end, uint64_t *value_out) {
    uint64_t result = 0;
    int shift = 0;
    const unsigned char *ptr = *buf_ptr;
    for (int i = 0; i < 10; ++i) {
        if (ptr >= buf_end) return 0;
        unsigned char byte = *ptr++;
        result |= (uint64_t)(byte & 0x7F) << shift;
        if (!(byte & 0x80U)) {
            *value_out = result;
            *buf_ptr = ptr;
            return 1;
        }
        shift += 7;
    }
    return 0; // Malformed: Varint is too long.
}

static inline void quicpro_proto_encode_fixed32(smart_str *buf, uint32_t value) {
    unsigned char temp_buf[4];
    temp_buf[0] = (unsigned char)(value);
    temp_buf[1] = (unsigned char)(value >> 8);
    temp_buf[2] = (unsigned char)(value >> 16);
    temp_buf[3] = (unsigned char)(value >> 24);
    smart_str_appendl(buf, (char*)temp_buf, 4);
}

static inline zend_bool quicpro_proto_decode_fixed32(const unsigned char **buf_ptr, const unsigned char *buf_end, uint32_t *value_out) {
    if (*buf_ptr + 4 > buf_end) return 0;
    const unsigned char *ptr = *buf_ptr;
    *value_out = ((uint32_t)ptr[0]) |
                 ((uint32_t)ptr[1] << 8) |
                 ((uint32_t)ptr[2] << 16) |
                 ((uint32_t)ptr[3] << 24);
    *buf_ptr += 4;
    return 1;
}

static inline void quicpro_proto_encode_fixed64(smart_str *buf, uint64_t value) {
    unsigned char temp_buf[8];
    temp_buf[0] = (unsigned char)(value);
    temp_buf[1] = (unsigned char)(value >> 8);
    temp_buf[2] = (unsigned char)(value >> 16);
    temp_buf[3] = (unsigned char)(value >> 24);
    temp_buf[4] = (unsigned char)(value >> 32);
    temp_buf[5] = (unsigned char)(value >> 40);
    temp_buf[6] = (unsigned char)(value >> 48);
    temp_buf[7] = (unsigned char)(value >> 56);
    smart_str_appendl(buf, (char*)temp_buf, 8);
}

static inline zend_bool quicpro_proto_decode_fixed64(const unsigned char **buf_ptr, const unsigned char *buf_end, uint64_t *value_out) {
    if (*buf_ptr + 8 > buf_end) return 0;
    const unsigned char *ptr = *buf_ptr;
    *value_out = ((uint64_t)ptr[0]) |
                 ((uint64_t)ptr[1] << 8)  |
                 ((uint64_t)ptr[2] << 16) |
                 ((uint64_t)ptr[3] << 24) |
                 ((uint64_t)ptr[4] << 32) |
                 ((uint64_t)ptr[5] << 40) |
                 ((uint64_t)ptr[6] << 48) |
                 ((uint64_t)ptr[7] << 56);
    *buf_ptr += 8;
    return 1;
}

static inline uint32_t quicpro_proto_zigzag_encode32(int32_t n) {
    return (uint32_t)((n << 1) ^ (n >> 31));
}

static inline int32_t quicpro_proto_zigzag_decode32(uint32_t n) {
    return (int32_t)((n >> 1) ^ (-(int32_t)(n & 1)));
}

static inline uint64_t quicpro_proto_zigzag_encode64(int64_t n) {
    return (uint64_t)((n << 1) ^ (n >> 63));
}

static inline int64_t quicpro_proto_zigzag_decode64(uint64_t n) {
    return (int64_t)((n >> 1) ^ (-(int64_t)(n & 1)));
}

#endif /* QUICPRO_PROTO_INTERNAL_H */