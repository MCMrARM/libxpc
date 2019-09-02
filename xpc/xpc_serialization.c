#include "xpc_serialization.h"
#include "xpc_internal.h"
#include <string.h>

#define XPC_TYPE_SIZE 4
#define XPC_TYPE_TYPE uint32_t

#define XPC_STRING_PAD_LEN(len) (((len + 1) + 3) / 4 * 4)

static size_t _xpc_dictionary_serialized_size(xpc_object_t obj);

size_t xpc_serialized_size(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    switch (v->type) {
        case XPC_BOOL:
            return XPC_TYPE_SIZE + sizeof(uint32_t);
        case XPC_INT64:
            return XPC_TYPE_SIZE + sizeof(int64_t);
        case XPC_UINT64:
            return XPC_TYPE_SIZE + sizeof(uint64_t);
        case XPC_DOUBLE:
            return XPC_TYPE_SIZE + sizeof(double);
        case XPC_DICTIONARY:
            return _xpc_dictionary_serialized_size(obj);
        default:
            return 0;
    }
}

static size_t _xpc_dictionary_serialized_size(xpc_object_t obj) {
    size_t ret = XPC_TYPE_SIZE + sizeof(uint32_t) * 2;
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    int i;
    struct xpc_dict_el *el;
    for (i = 0; i < XPC_DICT_NBUCKETS; ++i) {
        el = dict->buckets[i];
        while (el) {
            ret += XPC_STRING_PAD_LEN(el->key_length);
            ret += xpc_serialized_size(el->value);
            el = el->next;
        }
    }
    return ret;
}

#define XPC_WRITE(type, value) *((type *) buf) = value; buf += sizeof(type);

static size_t _xpc_dictionary_serialize(xpc_object_t obj, uint8_t *buf);

size_t xpc_serialize(xpc_object_t o, uint8_t *buf) {
    uint8_t *const buf_i = buf;
    struct xpc_value *v = (struct xpc_value *) o;
    switch (v->type) {
        case XPC_BOOL:
            XPC_WRITE(XPC_TYPE_TYPE, (XPC_BOOL << 12))
            XPC_WRITE(uint32_t, XPC_VALUE(v, bool))
            break;
        case XPC_INT64:
            XPC_WRITE(XPC_TYPE_TYPE, (XPC_INT64 << 12))
            XPC_WRITE(int64_t, XPC_VALUE(v, int64_t))
            break;
        case XPC_UINT64:
            XPC_WRITE(XPC_TYPE_TYPE, (XPC_UINT64 << 12))
            XPC_WRITE(uint64_t, XPC_VALUE(v, uint64_t))
            break;
        case XPC_DOUBLE:
            XPC_WRITE(XPC_TYPE_TYPE, (XPC_DOUBLE << 12))
            XPC_WRITE(double, XPC_VALUE(v, double))
            break;
        case XPC_DICTIONARY:
            return _xpc_dictionary_serialize(o, buf);
        default:
            break;
    }
    return buf_i - buf;
}

static size_t _xpc_dictionary_serialize(xpc_object_t obj, uint8_t *buf) {
    uint8_t *const buf_i = buf;
    uint32_t *size_ptr;
    size_t key_size;
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    int i;
    struct xpc_dict_el *el;
    XPC_WRITE(XPC_TYPE_TYPE, (XPC_DICTIONARY << 12))
    size_ptr = (uint32_t *) buf;
    XPC_WRITE(uint32_t, 0);
    XPC_WRITE(uint32_t, dict->count);
    for (i = 0; i < XPC_DICT_NBUCKETS; ++i) {
        el = dict->buckets[i];
        while (el) {
            key_size = XPC_STRING_PAD_LEN(el->key_length);
            memcpy(buf, el->key, el->key_length + 1);
            memset(&buf[el->key_length + 1], 0, key_size - (el->key_length + 1));
            buf += key_size;
            buf += xpc_serialize(obj, buf);
            el = el->next;
        }
    }
    *size_ptr = buf - buf_i;
    return buf - buf_i;
}