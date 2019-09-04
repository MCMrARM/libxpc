#include <xpc/xpc_serialization.h>
#include "xpc_internal.h"
#include <string.h>
#include <stdio.h>

typedef uint32_t xpc_s_type_t;

#define XPC_BIN_MAGIC 0x42133742
#define XPC_BIN_VERSION 5

#define XPC_DATA_PAD_SIZE(len) ((len+ 3) / 4 * 4)
#define XPC_STRING_PAD_LEN(len) (((len + 1) + 3) / 4 * 4)
#define XPC_SERIALIZED_TYPE(typ) (typ << 12)

static size_t _xpc_dictionary_serialized_size(xpc_object_t obj);

static size_t _xpc_serialized_size(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    switch (v->type) {
        case XPC_BOOL:
            return sizeof(xpc_s_type_t) + sizeof(uint32_t);
        case XPC_INT64:
            return sizeof(xpc_s_type_t) + sizeof(int64_t);
        case XPC_UINT64:
            return sizeof(xpc_s_type_t) + sizeof(uint64_t);
        case XPC_DOUBLE:
            return sizeof(xpc_s_type_t) + sizeof(double);
        case XPC_DATA:
            return sizeof(xpc_s_type_t) + sizeof(int32_t) + XPC_DATA_PAD_SIZE(xpc_data_get_length(obj));
        case XPC_STRING:
            return sizeof(xpc_s_type_t) + sizeof(int32_t) + XPC_STRING_PAD_LEN(xpc_string_get_length(obj));
        case XPC_DICTIONARY:
            return _xpc_dictionary_serialized_size(obj);
        default:
            return 0;
    }
}

static size_t _xpc_dictionary_serialized_size(xpc_object_t obj) {
    size_t ret = sizeof(xpc_s_type_t) + sizeof(uint32_t) * 2;
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    int i;
    struct xpc_dict_el *el;
    for (i = 0; i < XPC_DICT_NBUCKETS; ++i) {
        el = dict->buckets[i];
        while (el) {
            ret += XPC_STRING_PAD_LEN(el->key_length);
            ret += _xpc_serialized_size(el->value);
            el = el->next;
        }
    }
    return ret;
}

size_t xpc_serialized_size(xpc_object_t obj) {
    return _xpc_serialized_size(obj) + sizeof(uint32_t) * 2;
}

#define XPC_WRITE(type, value) *((type *) buf) = value; buf += sizeof(type);

static size_t _xpc_dictionary_serialize(xpc_object_t obj, uint8_t *buf);

static size_t _xpc_serialize(xpc_object_t o, uint8_t *buf) {
    size_t len;
    uint8_t *const buf_i = buf;
    struct xpc_value *v = (struct xpc_value *) o;
    switch (v->type) {
        case XPC_BOOL:
            XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_BOOL))
            XPC_WRITE(uint32_t, XPC_VALUE(v, bool))
            break;
        case XPC_INT64:
            XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_INT64))
            XPC_WRITE(int64_t, XPC_VALUE(v, int64_t))
            break;
        case XPC_UINT64:
            XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_UINT64))
            XPC_WRITE(uint64_t, XPC_VALUE(v, uint64_t))
            break;
        case XPC_DOUBLE:
            XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_DOUBLE))
            XPC_WRITE(double, XPC_VALUE(v, double))
            break;
        case XPC_DATA:
            XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_DATA))
            len = xpc_data_get_length(o);
            XPC_WRITE(uint32_t, len)
            memcpy(buf, xpc_data_get_bytes_ptr(o), len);
            buf += len;
            break;
        case XPC_STRING:
            XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_STRING))
            len = xpc_string_get_length(o) + 1;
            XPC_WRITE(uint32_t, len)
            memcpy(buf, xpc_string_get_string_ptr(o), len);
            buf += len;
            break;
        case XPC_DICTIONARY:
            return _xpc_dictionary_serialize(o, buf);
        default:
            break;
    }
    return buf - buf_i;
}

static size_t _xpc_dictionary_serialize(xpc_object_t obj, uint8_t *buf) {
    uint8_t *const buf_i = buf;
    uint32_t *size_ptr;
    size_t key_size;
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    int i;
    struct xpc_dict_el *el;
    XPC_WRITE(xpc_s_type_t, XPC_SERIALIZED_TYPE(XPC_DICTIONARY))
    size_ptr = (uint32_t *) buf;
    XPC_WRITE(uint32_t, 0)
    XPC_WRITE(uint32_t, dict->count)
    for (i = 0; i < XPC_DICT_NBUCKETS; ++i) {
        el = dict->buckets[i];
        while (el) {
            key_size = XPC_STRING_PAD_LEN(el->key_length);
            memcpy(buf, el->key, el->key_length + 1);
            memset(&buf[el->key_length + 1], 0, key_size - (el->key_length + 1));
            buf += key_size;
            buf += _xpc_serialize(el->value, buf);
            el = el->next;
        }
    }
    *size_ptr = buf - (uint8_t *) (size_ptr + 1);
    return buf - buf_i;
}

size_t xpc_serialize(xpc_object_t o, uint8_t *buf) {
    XPC_WRITE(uint32_t, XPC_BIN_MAGIC);
    XPC_WRITE(uint32_t, XPC_BIN_VERSION);
    return _xpc_serialize(o, buf) + sizeof(__uint32_t) * 2;
}

#define XPC_READ(type) ({ off += sizeof(type); off <= len ? *((type *) (&buf[off - sizeof(type)])) : 0; })

static xpc_object_t _xpc_deserialize_dictionary(const uint8_t *buf, size_t *offp, size_t len);

static xpc_object_t _xpc_deserialize(const uint8_t *buf, size_t *offp, size_t len) {
    size_t tlen, off = *offp;
    xpc_object_t ret = NULL;
    xpc_s_type_t type;
    type = XPC_READ(xpc_s_type_t) >> 12;
    switch (type) {
        case XPC_BOOL:
            ret = xpc_bool_create(XPC_READ(int32_t));
            break;
        case XPC_INT64:
            ret = xpc_int64_create(XPC_READ(int64_t));
            break;
        case XPC_UINT64:
            ret = xpc_uint64_create(XPC_READ(uint64_t));
            break;
        case XPC_DOUBLE:
            ret = xpc_double_create(XPC_READ(double));
            break;
        case XPC_DATA:
            tlen = XPC_READ(int32_t);
            ret = xpc_data_create(&buf[off], tlen);
            off += tlen;
            break;
        case XPC_STRING:
            tlen = XPC_READ(int32_t);
            if (tlen > 0) {
                ret = xpc_string_create_with_length((char *) &buf[off], tlen - 1);
                off += tlen;
            }
            break;
        case XPC_DICTIONARY:
            *offp = off;
            return _xpc_deserialize_dictionary(buf, offp, len);
    }
    *offp = off;
    return ret;
}

static xpc_object_t _xpc_deserialize_dictionary(const uint8_t *buf, size_t *offp, size_t len) {
    size_t off = *offp;
    size_t r_size, r_cnt, m_len, key_size;
    char *key;
    xpc_object_t ret, val;
    ret = xpc_dictionary_create(NULL, NULL, 0);
    r_size = XPC_READ(uint32_t);
    m_len = off + r_size;
    if (len > m_len)
        len = m_len;
    r_cnt = XPC_READ(uint32_t);
    while (r_cnt--) {
        key = (char *) &buf[off];
        key_size = strnlen(key, len - off);
        off += XPC_STRING_PAD_LEN(key_size);
        val = _xpc_deserialize(buf, &off, len);
        xpc_dictionary_set_value(ret, key, val);
    }
    *offp = off;
    return ret;
}

xpc_object_t xpc_deserialize(const uint8_t *buf, size_t len) {
    size_t off = 0;
    uint32_t magic = XPC_READ(uint32_t);
    uint32_t version = XPC_READ(uint32_t);
    if (magic != XPC_BIN_MAGIC || version != XPC_BIN_VERSION)
        return NULL;
    return _xpc_deserialize(buf, &off, len);
}