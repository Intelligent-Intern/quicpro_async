/*
 * include/proto.h – Public Interface for Quicpro\Proto Serialization
 * ====================================================================
 *
 * This header file declares the function prototypes for the Quicpro\Proto
 * serialization and deserialization functionalities within the php-quicpro_async
 * extension. It provides a Protobuf-like mechanism for defining message
 * schemas and efficiently encoding/decoding PHP data structures to/from a
 * compact binary wire format.
 *
 * Key Operations:
 * - Defining named schemas with typed fields, tags, and options (e.g., optional, repeated, default).
 * - Encoding PHP arrays or objects into a binary string based on a defined schema.
 * - Decoding a binary string back into a PHP array or stdClass object based on a schema.
 *
 * This C-native implementation aims for high performance, minimizing overhead
 * compared to pure PHP serialization solutions, especially for network communication
 * via MCP (Model Context Protocol).
 *
 * Argument information (arginfo) for these PHP_FUNCTIONs is in php_quicpro_arginfo.h.
 * The internal compiled schema representation ("opcode templates") and the binary
 * wire format are implementation details of proto.c.
 */

#ifndef QUICPRO_PROTO_H
#define QUICPRO_PROTO_H

#include <php.h> // Required for PHP_FUNCTION macro, zval, zend_string, etc.

/*
 * PHP_FUNCTION(quicpro_proto_define_schema);
 * ------------------------------------------
 * Defines and compiles a message schema for later use in encoding/decoding.
 * The schema definition is provided as a PHP array.
 *
 * Userland Signature (conceptual, actual in php_quicpro_arginfo.h):
 * bool Quicpro\Proto::defineSchema(string $schemaName, array $schemaDefinition)
 *
 * $schemaDefinition Example:
 * [
 * // Field name (or tag number if names are optional in C) => [options]
 * 'user_id' => ['type' => 'int64',  'tag' => 1, 'required' => true],
 * 'username'=> ['type' => 'string', 'tag' => 2, 'required' => true],
 * 'email'   => ['type' => 'string', 'tag' => 3, 'optional' => true],
 * 'roles'   => ['type' => 'repeated_string', 'tag' => 4], // Array of strings
 * 'profile' => ['type' => 'UserProfileMessage', 'tag' => 5, 'optional' => true], // Nested message
 * 'status'  => ['type' => 'enum_StatusType', 'tag' => 6, 'default' => 'ACTIVE_ENUM_VALUE'] // Enums map to int/string
 * ]
 * Supported types: 'bool', 'int32', 'int64', 'uint32', 'uint64', 'sint32', 'sint64',
 * 'fixed32', 'fixed64', 'sfixed32', 'sfixed64', 'float', 'double',
 * 'string', 'bytes', 'repeated_<type>' (e.g., 'repeated_int32', 'repeated_UserProfileMessage'),
 * '<SchemaName>' (for nested messages, referencing another defined schema).
 *
 * Returns: true on successful definition and compilation of the schema, false on error
 * (e.g., invalid schema structure, duplicate schema name if not allowed to redefine).
 * Throws ProtoException on critical errors.
 */
PHP_FUNCTION(quicpro_proto_define_schema);

/*
 * PHP_FUNCTION(quicpro_proto_encode);
 * -----------------------------------
 * Encodes a PHP array or object into a binary string using a predefined schema.
 *
 * Userland Signature:
 * string|false Quicpro\Proto::encode(string $schemaName, array|object $phpData)
 *
 * $phpData: An associative array or stdClass object matching the structure defined
 * by the $schemaName. Extra fields in $phpData not in the schema are typically ignored.
 * Missing required fields (if schema enforces this) will result in an error.
 *
 * Returns: A binary string representing the serialized data on success,
 * or false on failure (e.g., schema not found, data validation against schema fails).
 * Throws ProtoException on critical errors.
 */
PHP_FUNCTION(quicpro_proto_encode);

/*
 * PHP_FUNCTION(quicpro_proto_decode);
 * -----------------------------------
 * Decodes a binary string into a PHP associative array (or stdClass) using a predefined schema.
 *
 * Userland Signature:
 * array|object|false Quicpro\Proto::decode(string $schemaName, string $binaryData [, bool $decodeAsObject = false])
 *
 * $binaryData: The binary string to deserialize.
 * $decodeAsObject: If true, returns stdClass object; otherwise, an associative array (default).
 *
 * Returns: A PHP associative array or stdClass object on success,
 * or false on failure (e.g., schema not found, malformed binary data, validation fails).
 * Throws ProtoException on critical errors.
 */
PHP_FUNCTION(quicpro_proto_decode);

/*
 * PHP_FUNCTION(quicpro_proto_is_defined);
 * ---------------------------------------
 * Checks if a schema with the given name has already been defined.
 * Useful for idempotent schema definition in bootstrap files.
 *
 * Userland Signature:
 * bool Quicpro\Proto::isDefined(string $schemaName)
 *
 * Returns: true if the schema is defined, false otherwise.
 */
PHP_FUNCTION(quicpro_proto_is_defined);

/*
 * PHP_FUNCTION(quicpro_proto_get_defined_schemas);
 * ------------------------------------------------
 * Retrieves a list of all currently defined schema names.
 * Useful for debugging or introspection.
 *
 * Userland Signature:
 * array Quicpro\Proto::getDefinedSchemas(void)
 *
 * Returns: An array of strings, where each string is a defined schema name.
 */
PHP_FUNCTION(quicpro_proto_get_defined_schemas);


#endif /* QUICPRO_PROTO_H */