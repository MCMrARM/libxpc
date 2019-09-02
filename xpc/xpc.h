#ifndef XPC_H
#define XPC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void *xpc_object_t;

enum xpc_value_type {
    XPC_NULL = 1,
    XPC_BOOL = 2,
    XPC_INT64 = 3,
    XPC_UINT64 = 4,
    XPC_DOUBLE = 5,
    XPC_POINTER = 6,
    XPC_DATE = 7,
    XPC_DATA = 8,
    XPC_STRING = 9,
    XPC_UUID = 10,

    XPC_ARRAY = 14,
    XPC_DICTIONARY = 15
};

void xpc_free(xpc_object_t obj);

xpc_object_t xpc_bool_create(bool value);
xpc_object_t xpc_int64_create(int64_t value);
xpc_object_t xpc_uint64_create(uint64_t value);
xpc_object_t xpc_double_create(double value);

xpc_object_t xpc_dictionary_create(const char **keys, const xpc_object_t *values, size_t count);
xpc_object_t xpc_dictionary_get_value(xpc_object_t obj, const char *key);
void xpc_dictionary_set_value(xpc_object_t obj, const char *key, xpc_object_t value);

#endif //XPC_H
