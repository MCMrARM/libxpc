#ifndef XPC_SERIALIZATION_H
#define XPC_SERIALIZATION_H

#include "xpc.h"

size_t xpc_serialized_size(xpc_object_t o);
size_t xpc_serialize(xpc_object_t o, uint8_t *buf);

xpc_object_t xpc_deserialize(uint8_t *buf, size_t len);

#endif //XPC_SERIALIZATION_H
