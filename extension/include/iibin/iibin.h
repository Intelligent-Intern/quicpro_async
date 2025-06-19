/*
 * include/proto.h – Public Interface for Quicpro\Proto Serialization
 * ====================================================================
 *
 * This header file declares the function prototypes for the Quicpro\Proto
 * serialization and deserialization functionalities within the php-quicpro_async
 * extension. It provides a Protobuf-like mechanism for defining message
 * schemas and enum types, and efficiently encoding/decoding PHP data structures
 * to/from a compact binary wire format.
 *
 * Key Operations:
 * - Defining named schemas and enums.
 * - Encoding PHP arrays or objects into a binary string based on a defined schema.
 * - Decoding a binary string back into a PHP array or stdClass object based on a schema.
 *
 * Implementations reside in proto_schema.c, proto_encoding.c, and proto_decoding.c.
 * Argument information (arginfo) is in php_quicpro_arginfo.h.
 */

#ifndef QUICPRO_PROTO_H
#define QUICPRO_PROTO_H

#include <php.h> /* Required for PHP_FUNCTION macro, zval, zend_string, etc. */

/*
 * PHP_FUNCTION(quicpro_proto_define_enum);
 * ----------------------------------------
 * Defines an enumeration type for use in message schemas.
 * Enums are typically transmitted as integers.
 *
 * Userland Signature (conceptual, actual in php_quicpro_arginfo.h):
 * bool Quicpro\Proto::defineEnum(string $enumName, array $enumValues)
 *
 * $enumValues Example:
 * [
 * 'STATUS_UNSPECIFIED' => 0,
 * 'STATUS_ACTIVE'      => 1,
 * 'STATUS_PENDING'     => 2,
 * 'STATUS_ERROR'       => 10 // Values can be non-contiguous
 * ]
 *
 * Returns: true on successful definition, false on error. Throws ProtoException.
 */
PHP_FUNCTION(quicpro_proto_define_enum);

/*
 * PHP_FUNCTION(quicpro_proto_define_schema);
 * ------------------------------------------
 * Defines and compiles a message schema for later use in encoding/decoding.
 *
 * Userland Signature (conceptual, actual in php_quicpro_arginfo.h):
 * bool Quicpro\Proto::defineSchema(string $schemaName, array $schemaDefinition)
 *
 * $schemaDefinition Example:
 * [
 * 'status' => ['type' => 'MyStatusEnum', 'tag' => 1, 'default' => 'STATUS_ACTIVE'], // Using a defined enum
 * 'id'     => ['type' => 'int64',  'tag' => 2]
 * ]
 *
 * Returns: true on successful definition, false on error. Throws ProtoException.
 */
PHP_FUNCTION(quicpro_proto_define_schema);

/*
 * PHP_FUNCTION(quicpro_proto_encode);
 * -----------------------------------
 * Encodes a PHP array or object into a binary string using a predefined schema.
 *
 * Userland Signature:
 * string|false Quicpro\Proto::encode(string $schemaName, array|object $phpData)
 */
PHP_FUNCTION(quicpro_proto_encode);

/*
 * PHP_FUNCTION(quicpro_proto_decode);
 * -----------------------------------
 * Decodes a binary string into a PHP associative array (or stdClass) using a predefined schema.
 *
 * Userland Signature:
 * array|object|false Quicpro\Proto::decode(string $schemaName, string $binaryData [, bool $decodeAsObject = false])
 */
PHP_FUNCTION(quicpro_proto_decode);

/*
 * PHP_FUNCTION(quicpro_proto_is_defined);
 * ---------------------------------------
 * Checks if a schema OR enum with the given name has already been defined.
 * To check for a specific type (schema or enum), use the more specific functions below.
 *
 * Userland Signature:
 * bool Quicpro\Proto::isDefined(string $name)
 */
PHP_FUNCTION(quicpro_proto_is_defined); // Might need refinement or separate isSchemaDefined/isEnumDefined

/*
 * PHP_FUNCTION(quicpro_proto_is_schema_defined);
 * ---------------------------------------------
 * Checks if a message schema with the given name has already been defined.
 *
 * Userland Signature:
 * bool Quicpro\Proto::isSchemaDefined(string $schemaName)
 */
PHP_FUNCTION(quicpro_proto_is_schema_defined);

/*
 * PHP_FUNCTION(quicpro_proto_is_enum_defined);
 * -------------------------------------------
 * Checks if an enum with the given name has already been defined.
 *
 * Userland Signature:
 * bool Quicpro\Proto::isEnumDefined(string $enumName)
 */
PHP_FUNCTION(quicpro_proto_is_enum_defined);


/*
 * PHP_FUNCTION(quicpro_proto_get_defined_schemas);
 * ------------------------------------------------
 * Retrieves a list of all currently defined message schema names.
 *
 * Userland Signature:
 * array Quicpro\Proto::getDefinedSchemas(void)
 */
PHP_FUNCTION(quicpro_proto_get_defined_schemas);

/*
 * PHP_FUNCTION(quicpro_proto_get_defined_enums);
 * ----------------------------------------------
 * Retrieves a list of all currently defined enum type names.
 *
 * Userland Signature:
 * array Quicpro\Proto::getDefinedEnums(void)
 */
PHP_FUNCTION(quicpro_proto_get_defined_enums);


#endif /* QUICPRO_PROTO_H */