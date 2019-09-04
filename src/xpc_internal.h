#ifndef XPC_INTERNAL_H
#define XPC_INTERNAL_H

#include <xpc/xpc.h>

struct xpc_value {
    enum xpc_value_type type;
    char value[];
};
#define XPC_VALUE(v, type) (*((type *) v->value))

struct xpc_value_varlen {
    enum xpc_value_type type;
    size_t size;
    char value[];
};

#define XPC_DICT_NBUCKETS 8
#define XPC_DICT_BUCKET(hash) (hash % XPC_DICT_NBUCKETS)

struct xpc_dict_el;
struct xpc_dict {
    enum xpc_value_type type;
    size_t count;
    struct xpc_dict_el *buckets[XPC_DICT_NBUCKETS];
};
struct xpc_dict_el {
    unsigned long hash;
    xpc_object_t value;
    struct xpc_dict_el *prev, *next;
    size_t key_length;
    char key[];
};

struct xpc_array {
    enum xpc_value_type type;
    size_t count, mem_count;
    xpc_object_t **value;
};

#endif //XPC_INTERNAL_H
