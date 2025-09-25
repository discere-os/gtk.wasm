#include <gtk/gtk.h>
#include <stdio.h>

const char* gtk_wasm_version(void) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "%d.%d.%d", gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version());
  return buf;
}

