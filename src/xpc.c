#include <xpc/xpc.h>
#include "xpc_internal.h"
#include <string.h>
#include <malloc.h>
#include <math.h>

static void _xpc_dictionary_free(xpc_object_t obj);

static struct xpc_value *_xpc_alloc_value(enum xpc_value_type type, size_t data_size) {
    struct xpc_value *val = malloc(sizeof(struct xpc_value) + data_size);
    val->type = type;
    return val;
}

void xpc_free(xpc_object_t obj) {
    struct xpc_value *v;
    if (!obj)
        return;

    v = (struct xpc_value *) obj;
    if (v->type == XPC_DICTIONARY)
        _xpc_dictionary_free(v);
    else
        free(v);
}

xpc_type_t xpc_get_type(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    return v->type;
}

xpc_object_t xpc_bool_create(bool value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_BOOL, sizeof(bool));
    XPC_VALUE(v, bool) = value;
    return v;
}
bool xpc_bool_get_value(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    return XPC_VALUE(v, bool);
}
xpc_object_t xpc_int64_create(int64_t value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_INT64, sizeof(int64_t));
    XPC_VALUE(v, int64_t) = value;
    return v;
}
int64_t xpc_int64_get_value(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    return XPC_VALUE(v, int64_t);
}
xpc_object_t xpc_uint64_create(uint64_t value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_UINT64, sizeof(int64_t));
    XPC_VALUE(v, uint64_t) = value;
    return v;
}
uint64_t xpc_uint64_get_value(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    return XPC_VALUE(v, uint64_t);
}
xpc_object_t xpc_double_create(double value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_DOUBLE, sizeof(int64_t));
    XPC_VALUE(v, double) = value;
    return v;
}
double xpc_double_get_value(xpc_object_t obj) {
    struct xpc_value *v = (struct xpc_value *) obj;
    return XPC_VALUE(v, double);
}

static struct xpc_value_varlen *_xpc_alloc_value_varlen(enum xpc_value_type type, size_t data_size) {
    struct xpc_value_varlen *val = malloc(sizeof(struct xpc_value_varlen) + data_size);
    val->type = type;
    val->size = data_size;
    return val;
}
xpc_object_t xpc_data_create(const void *value, size_t length) {
    struct xpc_value_varlen *v = _xpc_alloc_value_varlen(XPC_DATA, length);
    memcpy(v->value, value, length);
    return v;
}
size_t xpc_data_get_length(xpc_object_t obj) {
    struct xpc_value_varlen *v = (struct xpc_value_varlen *) obj;
    return v->size;
}
size_t xpc_data_get_bytes(xpc_object_t obj, void *ptr, size_t off, size_t len) {
    struct xpc_value_varlen *v = (struct xpc_value_varlen *) obj;
    if (len > v->size - off)
        len = v->size - off;
    memcpy(ptr, &v->value[off], len);
    return len;
}
const void *xpc_data_get_bytes_ptr(xpc_object_t obj) {
    struct xpc_value_varlen *v = (struct xpc_value_varlen *) obj;
    return v->value;
}
xpc_object_t xpc_string_create(const char *value) {
    size_t len = strlen(value);
    struct xpc_value_varlen *v = _xpc_alloc_value_varlen(XPC_STRING, len + 1);
    memcpy(v->value, value, len + 1);
    return v;
}
xpc_object_t xpc_string_create_with_length(const char *value, size_t len) {
    struct xpc_value_varlen *v = _xpc_alloc_value_varlen(XPC_STRING, len + 1);
    memcpy(v->value, value, len + 1);
    return v;
}
size_t xpc_string_get_length(xpc_object_t obj) {
    struct xpc_value_varlen *v = (struct xpc_value_varlen *) obj;
    return v->size - 1;
}
const char *xpc_string_get_string_ptr(xpc_object_t obj) {
    struct xpc_value_varlen *v = (struct xpc_value_varlen *) obj;
    return v->value;
}

static unsigned long _xpc_dictionary_hash_key(const char *str) {
    unsigned long hash = 5381;
    while (*str) {
        hash = (hash * 33) + (unsigned char) (*str);
        ++str;
    }
    return hash;
}
xpc_object_t xpc_dictionary_create(const char **keys, const xpc_object_t *values, size_t count) {
    size_t i;
    struct xpc_dict *dict = malloc(sizeof(struct xpc_dict));
    dict->type = XPC_DICTIONARY;
    dict->count = count;
    memset(dict->buckets, 0, sizeof(dict->buckets));
    for (i = 0; i < count; i++)
        xpc_dictionary_set_value(dict, keys[i], values[i]);
    return dict;
}
static void _xpc_dictionary_free(xpc_object_t obj) {
    int i;
    struct xpc_dict_el *el, *tel;
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    for (i = 0; i < XPC_DICT_NBUCKETS; ++i) {
        el = dict->buckets[i];
        while (el) {
            xpc_free(el->value);
            tel = el;
            el = el->next;
            free(tel);
        }
    }
}
static struct xpc_dict_el *xpc_dictionary_find_el(xpc_object_t obj, const char *key, unsigned long key_hash) {
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    struct xpc_dict_el *el;
    el = dict->buckets[XPC_DICT_BUCKET(key_hash)];
    while (el) {
        if (el->hash == key_hash && strcmp(el->key, key) == 0)
            return el;
        el = el->next;
    }
    return NULL;
}
xpc_object_t xpc_dictionary_get_value(xpc_object_t obj, const char *key) {
    unsigned long key_hash = _xpc_dictionary_hash_key(key);
    struct xpc_dict_el *el = xpc_dictionary_find_el(obj, key, key_hash);
    return el ? el->value : NULL;
}
void xpc_dictionary_set_value(xpc_object_t obj, const char *key, xpc_object_t value) {
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    unsigned long key_hash = _xpc_dictionary_hash_key(key);
    size_t key_length;
    struct xpc_dict_el *el = xpc_dictionary_find_el(obj, key, key_hash);

    if (!value) {
        if (el) {
            if (el->prev)
                el->prev->next = el->next;
            if (el->next)
                el->next->prev = el->prev;
            xpc_free(el->value);
            free(el);
            --dict->count;
        }
        return;
    }
    if (el) {
        xpc_free(el->value);
        el->value = value;
    } else {
        key_length = strlen(key);
        el = malloc(sizeof(struct xpc_dict_el) + key_length + 1);
        el->prev = NULL;
        el->next = dict->buckets[XPC_DICT_BUCKET(key_hash)];
        if (el->next)
            el->next->prev = el;
        el->key_length = key_length;
        memcpy(el->key, key, key_length + 1);
        el->hash = key_hash;
        el->value = value;
        dict->buckets[XPC_DICT_BUCKET(key_hash)] = el;
        ++dict->count;
    }
}

bool xpc_dictionary_get_bool(xpc_object_t obj, const char *key) {
    xpc_object_t o = xpc_dictionary_get_value(obj, key);
    return (xpc_get_type(o) == XPC_BOOL ? xpc_bool_get_value(o) : false);
}
int64_t xpc_dictionary_get_int64(xpc_object_t obj, const char *key) {
    xpc_object_t o = xpc_dictionary_get_value(obj, key);
    return (xpc_get_type(o) == XPC_INT64 ? xpc_int64_get_value(o) : 0);
}
uint64_t xpc_dictionary_get_uint64(xpc_object_t obj, const char *key) {
    xpc_object_t o = xpc_dictionary_get_value(obj, key);
    return (xpc_get_type(o) == XPC_UINT64 ? xpc_uint64_get_value(o) : 0);
}
double xpc_dictionary_get_double(xpc_object_t obj, const char *key) {
    xpc_object_t o = xpc_dictionary_get_value(obj, key);
    return (xpc_get_type(o) == XPC_DOUBLE ? xpc_double_get_value(o) : NAN);
}
const void *xpc_dictionary_get_data(xpc_object_t obj, const char *key, size_t *length) {
    xpc_object_t o = xpc_dictionary_get_value(obj, key);
    if (xpc_get_type(o) == XPC_DATA) {
        *length = xpc_data_get_length(o);
        return xpc_data_get_bytes_ptr(obj);
    }
    *length = 0;
    return NULL;
}
const char *xpc_dictionary_get_string(xpc_object_t obj, const char *key) {
    xpc_object_t o = xpc_dictionary_get_value(obj, key);
    return (xpc_get_type(o) == XPC_STRING ? xpc_string_get_string_ptr(o) : NULL);
}

void xpc_dictionary_set_bool(xpc_object_t obj, const char *key, bool value) {
    xpc_dictionary_set_value(obj, key, xpc_bool_create(value));
}
void xpc_dictionary_set_int64(xpc_object_t obj, const char *key, int64_t value) {
    xpc_dictionary_set_value(obj, key, xpc_int64_create(value));
}
void xpc_dictionary_set_uint64(xpc_object_t obj, const char *key, uint64_t value) {
    xpc_dictionary_set_value(obj, key, xpc_uint64_create(value));
}
void xpc_dictionary_set_double(xpc_object_t obj, const char *key, double value) {
    xpc_dictionary_set_value(obj, key, xpc_double_create(value));
}
void xpc_dictionary_set_data(xpc_object_t obj, const char *key, const void *value, size_t length) {
    xpc_dictionary_set_value(obj, key, xpc_data_create(value, length));
}
void xpc_dictionary_set_string(xpc_object_t obj, const char *key, const char *value) {
    xpc_dictionary_set_value(obj, key, xpc_string_create(value));
}