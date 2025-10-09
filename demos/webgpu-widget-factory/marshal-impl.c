// Minimal marshal function implementations for GObject signals
// These are called by GObject when no specific marshaller is provided

#include <stdarg.h>

// Forward declarations of GObject types
typedef struct _GClosure GClosure;
typedef struct _GValue GValue;

// Generic marshal function - called when no specific marshaller is set
// In our case, we don't use signals, so this is a no-op
void g_cclosure_marshal_generic(GClosure *closure,
                                  GValue *return_gvalue,
                                  unsigned int n_param_values,
                                  const GValue *param_values,
                                  void *invocation_hint,
                                  void *marshal_data) {
    // No-op: we don't use GObject signals in our rendering code
    // This function is referenced by Pango/Cairo but never actually called
    // in our use case since we're just doing pure rendering
}

// va_list variant of the generic marshaller
void g_cclosure_marshal_generic_va(GClosure *closure,
                                     GValue *return_value,
                                     void *instance,
                                     va_list args,
                                     void *marshal_data,
                                     int n_params,
                                     void *param_types) {
    // No-op: we don't use GObject signals in our rendering code
}

// pthread stub - not used in single-threaded WASM
int pthread_getname_np(void *thread, char *name, unsigned long len) {
    if (name && len > 0) name[0] = '\0';
    return 0;
}
