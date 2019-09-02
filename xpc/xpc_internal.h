#ifndef XPC_INTERNAL_H
#define XPC_INTERNAL_H

#include "xpc.h"

struct xpc_value {
    enum xpc_value_type type;
    char value[];
};
#define XPC_VALUE(v, type) (*((type *) v->value))

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
    char key[];
};

#endif //XPC_INTERNAL_H
