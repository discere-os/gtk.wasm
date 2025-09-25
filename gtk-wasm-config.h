/* GTK WASM build configuration fixes */
#ifndef GTK_WASM_CONFIG_H
#define GTK_WASM_CONFIG_H

/* Define G_GNUC_INTERNAL for visibility */
#ifndef G_GNUC_INTERNAL
#  ifdef __GNUC__
#    define G_GNUC_INTERNAL __attribute__((visibility("hidden")))
#  else
#    define G_GNUC_INTERNAL
#  endif
#endif

/* Fix other missing definitions */
#ifndef CAIRO_HAS_PNG_FUNCTIONS
#  define CAIRO_HAS_PNG_FUNCTIONS 0
#endif

#endif /* GTK_WASM_CONFIG_H */