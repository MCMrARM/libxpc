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
typedef enum xpc_value_type xpc_type_t;

void xpc_free(xpc_object_t obj);

xpc_type_t xpc_get_type(xpc_object_t obj);

xpc_object_t xpc_bool_create(bool value);
bool xpc_bool_get_value(xpc_object_t obj);
xpc_object_t xpc_int64_create(int64_t value);
int64_t xpc_int64_get_value(xpc_object_t obj);
xpc_object_t xpc_uint64_create(uint64_t value);
uint64_t xpc_uint64_get_value(xpc_object_t obj);
xpc_object_t xpc_double_create(double value);
double xpc_double_get_value(xpc_object_t obj);
xpc_object_t xpc_data_create(const void *value, size_t length);
size_t xpc_data_get_length(xpc_object_t obj);
size_t xpc_data_get_bytes(xpc_object_t obj, void *ptr, size_t off, size_t len);
const void *xpc_data_get_bytes_ptr(xpc_object_t obj);
xpc_object_t xpc_string_create(const char *value);
xpc_object_t xpc_string_create_with_length(const char *value, size_t len);
size_t xpc_string_get_length(xpc_object_t obj);
const char *xpc_string_get_string_ptr(xpc_object_t obj);

xpc_object_t xpc_dictionary_create(const char **keys, const xpc_object_t *values, size_t count);
xpc_object_t xpc_dictionary_get_value(xpc_object_t obj, const char *key);
void xpc_dictionary_set_value(xpc_object_t obj, const char *key, xpc_object_t value);

#endif //XPC_H
