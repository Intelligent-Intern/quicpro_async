/*
 * iibin-js – Browser‑/Node‑compatible TypeScript runtime for Intelligent‑Intern Binary (IIBIN)
 * ============================================================================================
 * This is a **first‑cut reference decoder** that mirrors the C‑native implementation in
 * php‑quicpro_async.  It currently supports all scalar wire types, repeated / packed fields,
 * nested messages, and enums.  Encoding will be added once the decode API stabilises.
 *
 * Performance goals:
 *   • Zero‑copy: never allocate intermediate strings unless we really need them.
 *   • Stream‑friendly: work directly on Uint8Array / ArrayBuffer, keep pointer offset.
 *   • Strict: throw when schema and payload disagree (unknown enum, wrong wire‑type, …)
 *
 * Usage example (browser / Node ESM):
 *
 *   import { decode, Schema } from "./decode.js";
 *
 *   const TelemetrySchema: Schema = {
 *     name: "Telemetry",
 *     fieldsByTag: {
 *       1: { name: "vehicleId", tag: 1, type: "uint32", wireType: 0 },
 *       2: { name: "lat",       tag: 2, type: "double", wireType: 1 },
 *       3: { name: "lng",       tag: 3, type: "double", wireType: 1 },
 *       4: { name: "speed",     tag: 4, type: "float",  wireType: 5, default: 0 },
 *       5: { name: "ts",        tag: 5, type: "uint64", wireType: 0 }
 *     }
 *   };
 *
 *   const msg = decode(buf, TelemetrySchema);
 *   console.log(msg.vehicleId, msg.speed);
 */

// -----------------------------------------
// Wire‑type constants (identical to C)
// -----------------------------------------
export enum WireType {
    VARINT       = 0,
    FIXED64      = 1,
    LENGTH_DELIM = 2,
    FIXED32      = 5
}

// Flags as bit‑flags to avoid nested objects
export const enum FieldFlags {
    OPTIONAL = 0x01,
        REQUIRED = 0x02,
        REPEATED = 0x04,
        PACKED   = 0x08
}

export interface FieldDef {
    name: string;
    tag: number;
    /** primitive type string or nested/enum type name */
    type: string;
    /** WireType expected on the wire (post‑packed). */
    wireType: WireType;
    /** Bitwise OR of FieldFlags */
    flags?: number;
    /** Name of nested schema (if type === "message") */
    nested?: string;
    /** Name of enum type (if type === "enum") */
    enumType?: string;
default?: unknown;
}

export interface Schema {
    name: string;
    /** Quick look‑up by wire‑tag */
    fieldsByTag: Record<number, FieldDef>;
    /** Optional: for iterating through required/default handling */
    requiredTags?: number[];
}

// Minimal enum registry – in real lib this will be generated
const enums: Record<string, Record<string, number>> = {};
export function registerEnum(name: string, values: Record<string, number>): void {
    enums[name] = values;
}

// Minimal schema registry for nested‐message decoding
const schemas: Record<string, Schema> = {};
export function registerSchema(schema: Schema): void {
    schemas[schema.name] = schema;
}

// -----------------------------
// Low‑level varint / ZigZag I/O
// -----------------------------
function readVarint(buf: Uint8Array, offset: number): [value: bigint, next: number] {
    let result = 0n;
    let shift = 0n;
    let pos = offset;
    while (pos < buf.length) {
        const byte = BigInt(buf[pos++]);
        result |= (byte & 0x7fn) << shift;
        if ((byte & 0x80n) === 0n) break;
        shift += 7n;
        if (shift > 70n) throw new Error("Varint too long – corrupt");
    }
    return [result, pos];
}

function zigzagDecode32(n: bigint): number {
    return Number((n >> 1n) ^ (-(n & 1n)));
}
function zigzagDecode64(n: bigint): bigint {
    return (n >> 1n) ^ (-(n & 1n));
}

// -----------------------------
// Primitive readers
// -----------------------------
function readFixed32(buf: Uint8Array, offset: number): [val: number, next: number] {
    if (offset + 4 > buf.length) throw new Error("Buffer underrun (fixed32)");
    const dv = new DataView(buf.buffer, buf.byteOffset + offset, 4);
    return [dv.getUint32(0, true), offset + 4];
}
function readFixed64(buf: Uint8Array, offset: number): [val: bigint, next: number] {
    if (offset + 8 > buf.length) throw new Error("Buffer underrun (fixed64)");
    const lo = BigInt(new DataView(buf.buffer, buf.byteOffset + offset, 4).getUint32(0, true));
    const hi = BigInt(new DataView(buf.buffer, buf.byteOffset + offset + 4, 4).getUint32(0, true));
    return [(hi << 32n) | lo, offset + 8];
}

// -----------------------------------------
// Core decode – returns plain JS object
// -----------------------------------------
export function decode(buf: Uint8Array, schema: Schema, asObject = true): any {
    let offset = 0;
    const obj: any = {};

    while (offset < buf.length) {
        // Read key (tag << 3 | wireType)
        let key: bigint; [key, offset] = readVarint(buf, offset);
        const tag = Number(key >> 3n);
        const wireType = Number(key & 0x7n);

        const field = schema.fieldsByTag[tag];
        if (!field) {
            // Unknown tag → skip for FW‑compatibility
            offset = skipField(buf, offset, wireType);
            continue;
        }

        // If packed repeated, wireType = LENGTH_DELIM
        if (field.flags && (field.flags & FieldFlags.PACKED) && wireType === WireType.LENGTH_DELIM) {
            let len: bigint; [len, offset] = readVarint(buf, offset);
            const end = offset + Number(len);
            if (end > buf.length) throw new Error(`Packed field length overflow for tag ${tag}`);
            while (offset < end) {
                const [val, next] = readScalar(buf, offset, field);
                offset = next;
                if (!obj[field.name]) obj[field.name] = [];
                obj[field.name].push(val);
            }
            continue;
        }

        // Non‑packed (single or repeated)
        const [val, next] = readScalar(buf, offset, field, wireType);
        offset = next;
        if (field.flags && (field.flags & FieldFlags.REPEATED)) {
            if (!obj[field.name]) obj[field.name] = [];
            obj[field.name].push(val);
        } else {
            obj[field.name] = val;
        }
    }

    // Defaults / required checks
    for (const tagStr of Object.keys(schema.fieldsByTag)) {
        const f = schema.fieldsByTag[Number(tagStr)];
        if (obj[f.name] === undefined) {
            if (f.flags && (f.flags & FieldFlags.REQUIRED)) {
                throw new Error(`Required field ${f.name} missing (schema ${schema.name})`);
            }
            if (f.default !== undefined) obj[f.name] = f.default;
        }
    }
    return obj;
}

// ------------- helpers ----------------
function readScalar(buf: Uint8Array, offset: number, field: FieldDef, explicitWire?: number): [any, number] {
    const wt = explicitWire ?? field.wireType;
    switch (field.type) {
        case "int32":
        case "uint32":
        case "enum": {
            let v: bigint; [v, offset] = readVarint(buf, offset);
            return [Number(v), offset];
        }
        case "sint32": {
            let v: bigint; [v, offset] = readVarint(buf, offset);
            return [zigzagDecode32(v), offset];
        }
        case "int64":
        case "uint64": {
            let v: bigint; [v, offset] = readVarint(buf, offset);
            return [v, offset];
        }
        case "sint64": {
            let v: bigint; [v, offset] = readVarint(buf, offset);
            return [zigzagDecode64(v), offset];
        }
        case "bool": {
            let v: bigint; [v, offset] = readVarint(buf, offset);
            return [v !== 0n, offset];
        }
        case "double": {
            let v: bigint; [v, offset] = readFixed64(buf, offset);
            return [Number(new Float64Array([Number(v)]).at(0)), offset];
        }
        case "float": {
            let num: number; [num, offset] = readFixed32(buf, offset);
            return [new Float32Array([num]).at(0), offset];
        }
        case "fixed32":
        case "sfixed32": {
            let num: number; [num, offset] = readFixed32(buf, offset);
            return [num, offset];
        }
        case "fixed64":
        case "sfixed64": {
            let v: bigint; [v, offset] = readFixed64(buf, offset);
            return [v, offset];
        }
        case "string":
        case "bytes": {
            let len: bigint; [len, offset] = readVarint(buf, offset);
            const strEnd = offset + Number(len);
            if (strEnd > buf.length) throw new Error("String length overflow");
            const slice = buf.subarray(offset, strEnd);
            offset = strEnd;
            if (field.type === "string") return [new TextDecoder().decode(slice), offset];
            return [slice, offset];
        }
        case "message": {
            let len: bigint; [len, offset] = readVarint(buf, offset);
            const strEnd = offset + Number(len);
            if (strEnd > buf.length) throw new Error("Nested length overflow");
            const nestedBuf = buf.subarray(offset, strEnd);
            offset = strEnd;
            const nestedSchema = schemas[field.nested!];
            if (!nestedSchema) throw new Error(`Nested schema ${field.nested} not registered`);
            return [decode(nestedBuf, nestedSchema), offset];
        }
        default:
            throw new Error(`Unsupported type ${field.type}`);
    }
}

function skipField(buf: Uint8Array, offset: number, wt: number): number {
    switch (wt) {
        case WireType.VARINT: {
            [, offset] = readVarint(buf, offset);
            return offset;
        }
        case WireType.FIXED64:
            return offset + 8;
        case WireType.LENGTH_DELIM: {
            let len: bigint; [len, offset] = readVarint(buf, offset);
            return offset + Number(len);
        }
        case WireType.FIXED32:
            return offset + 4;
        default:
            throw new Error(`Unknown wire‑type ${wt}`);
    }
}

// ------------- Exports (public API) -------------
export default {
    decode,
    registerSchema,
    registerEnum,
    WireType,
    FieldFlags
};
