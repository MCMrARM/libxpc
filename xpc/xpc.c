#include "xpc.h"
#include "xpc_internal.h"
#include <string.h>
#include <malloc.h>

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

xpc_object_t xpc_bool_create(bool value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_BOOL, sizeof(bool));
    XPC_VALUE(v, bool) = value;
    return v;
}
xpc_object_t xpc_int64_create(int64_t value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_INT64, sizeof(int64_t));
    XPC_VALUE(v, int64_t) = value;
    return v;
}
xpc_object_t xpc_uint64_create(uint64_t value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_UINT64, sizeof(int64_t));
    XPC_VALUE(v, uint64_t) = value;
    return v;
}
xpc_object_t xpc_double_create(double value) {
    struct xpc_value *v = _xpc_alloc_value(XPC_DOUBLE, sizeof(int64_t));
    XPC_VALUE(v, double) = value;
    return v;
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
    struct xpc_dict_el *el = xpc_dictionary_find_el(obj, key, key_hash);

    if (!value) {
        if (el) {
            if (el->prev)
                el->prev->next = el->next;
            if (el->next)
                el->next->prev = el->prev;
            xpc_free(el->value);
            free(el);
        }
        return;
    }
    if (el) {
        el->value = value;
    } else {
        el = malloc(sizeof(struct xpc_dict_el) + strlen(key) + 1);
        el->prev = NULL;
        el->next = dict->buckets[XPC_DICT_BUCKET(key_hash)];
        if (el->next)
            el->next->prev = el;
        strcpy(el->key, key);
        el->hash = key_hash;
        el->value = value;
        dict->buckets[XPC_DICT_BUCKET(key_hash)] = el;
    }
}
