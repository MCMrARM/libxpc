#ifndef XPC_DEBUG_H
#define XPC_DEBUG_H

#include "xpc.h"

typedef void (*xpc_debug_write)(const char *str);

void xpc_debug_print(xpc_object_t obj, xpc_debug_write out);
void xpc_debug_print_stdout(xpc_object_t obj);

#endif //XPC_DEBUG_H
