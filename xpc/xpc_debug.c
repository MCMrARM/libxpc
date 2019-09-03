#include "xpc_debug.h"
#include "xpc_internal.h"
#include <stdio.h>
#include <inttypes.h>

static void _xpc_debug_print_dict(xpc_object_t obj, xpc_debug_write out);

void xpc_debug_print(xpc_object_t obj, xpc_debug_write out) {
    char buf[64];
    struct xpc_value *val = (struct xpc_value *) obj;
    switch (val->type) {
        case XPC_BOOL:
            if (XPC_VALUE(val, bool))
                out("true");
            else
                out("false");
            break;
        case XPC_INT64:
            snprintf(buf, sizeof(buf), "%" PRIi64, XPC_VALUE(val, int64_t));
            out(buf);
            break;
        case XPC_UINT64:
            snprintf(buf, sizeof(buf), "%" PRIu64 "u", XPC_VALUE(val, uint64_t));
            out(buf);
            break;
        case XPC_DOUBLE:
            snprintf(buf, sizeof(buf), "%lf", XPC_VALUE(val, double));
            out(buf);
            break;
        case XPC_DATA:
            snprintf(buf, sizeof(buf), "<binary data %li>", xpc_data_get_length(obj));
            out(buf);
            break;
        case XPC_STRING:
            out("\"");
            out(xpc_string_get_string_ptr(obj));
            out("\"");
            break;
        case XPC_DICTIONARY:
            _xpc_debug_print_dict(obj, out);
            break;
    }
}

static void _xpc_debug_print_dict(xpc_object_t obj, xpc_debug_write out) {
    struct xpc_dict *dict = (struct xpc_dict *) obj;
    int i;
    struct xpc_dict_el *el;
    out("{");
    bool first = true;
    for (i = 0; i < XPC_DICT_NBUCKETS; ++i) {
        el = dict->buckets[i];
        while (el) {
            if (!first)
                out(", ");
            first = false;
            out(el->key);
            out(": ");
            xpc_debug_print(el->value, out);
            el = el->next;
        }
    }
    out("}");
}

static void _xpc_write_stdout(const char *str) {
    fputs(str, stdout);
}
void xpc_debug_print_stdout(xpc_object_t obj) {
    xpc_debug_print(obj, _xpc_write_stdout);
}