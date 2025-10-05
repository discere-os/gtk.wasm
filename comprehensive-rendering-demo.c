/* GTK Comprehensive Rendering Demo
 * Demonstrates all major rendering capabilities:
 * - Widgets (buttons, labels, entries, etc.)
 * - Text rendering with Pango
 * - Image display with GdkPixbuf
 * - Custom drawing with Cairo
 * - Layout managers
 * - Event handling
 *
 * Compile with Emscripten for WASM deployment
 */

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <pango/pango.h>
#include <math.h>

#define DRAWING_AREA_WIDTH 400
#define DRAWING_AREA_HEIGHT 300

/* Cairo custom drawing callback */
static void
draw_function (GtkDrawingArea *drawing_area,
              cairo_t        *cr,
              int             width,
              int             height,
              gpointer        user_data)
{
  /* Clear background */
  cairo_set_source_rgb (cr, 0.95, 0.95, 0.95);
  cairo_paint (cr);

  /* Draw gradient background */
  cairo_pattern_t *gradient = cairo_pattern_create_linear (0, 0, width, height);
  cairo_pattern_add_color_stop_rgba (gradient, 0, 0.2, 0.4, 0.8, 0.3);
  cairo_pattern_add_color_stop_rgba (gradient, 1, 0.8, 0.2, 0.4, 0.3);
  cairo_rectangle (cr, 10, 10, width - 20, height - 20);
  cairo_set_source (cr, gradient);
  cairo_fill (cr);
  cairo_pattern_destroy (gradient);

  /* Draw shapes - circle */
  cairo_set_source_rgb (cr, 1.0, 0.2, 0.2);
  cairo_arc (cr, width / 4, height / 4, 30, 0, 2 * M_PI);
  cairo_fill (cr);

  /* Draw rectangle */
  cairo_set_source_rgb (cr, 0.2, 1.0, 0.2);
  cairo_rectangle (cr, width / 2, height / 4 - 30, 60, 60);
  cairo_fill (cr);

  /* Draw triangle */
  cairo_set_source_rgb (cr, 0.2, 0.2, 1.0);
  cairo_move_to (cr, 3 * width / 4, height / 4 + 30);
  cairo_line_to (cr, 3 * width / 4 - 30, height / 4 - 30);
  cairo_line_to (cr, 3 * width / 4 + 30, height / 4 - 30);
  cairo_close_path (cr);
  cairo_fill (cr);

  /* Draw text with Pango */
  PangoLayout *layout = pango_cairo_create_layout (cr);
  pango_layout_set_text (layout, "GTK4 + Cairo + Pango", -1);

  PangoFontDescription *desc = pango_font_description_from_string ("Sans Bold 16");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_move_to (cr, 20, height / 2);
  pango_cairo_show_layout (cr, layout);
  g_object_unref (layout);

  /* Draw rotated text */
  cairo_save (cr);
  cairo_translate (cr, width / 2, 3 * height / 4);
  cairo_rotate (cr, -M_PI / 6);

  layout = pango_cairo_create_layout (cr);
  pango_layout_set_text (layout, "Rotated Text!", -1);
  desc = pango_font_description_from_string ("Serif Italic 14");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  cairo_set_source_rgba (cr, 1.0, 0.5, 0.0, 0.8);
  pango_cairo_show_layout (cr, layout);
  g_object_unref (layout);
  cairo_restore (cr);

  /* Draw bezier curves */
  cairo_set_source_rgba (cr, 0.5, 0.0, 0.5, 0.7);
  cairo_set_line_width (cr, 3);
  cairo_move_to (cr, 20, height - 40);
  cairo_curve_to (cr, width / 3, height - 80, 2 * width / 3, height - 10, width - 20, height - 40);
  cairo_stroke (cr);
}

/* Button click handlers */
static void
on_button_clicked (GtkButton *button, gpointer user_data)
{
  GtkLabel *label = GTK_LABEL (user_data);
  const char *text = gtk_button_get_label (button);
  char *message = g_strdup_printf ("Button '%s' clicked!", text);
  gtk_label_set_text (label, message);
  g_free (message);
}

static void
on_entry_activated (GtkEntry *entry, gpointer user_data)
{
  GtkLabel *label = GTK_LABEL (user_data);
  const char *text = gtk_editable_get_text (GTK_EDITABLE (entry));
  char *message = g_strdup_printf ("Entry text: %s", text);
  gtk_label_set_text (label, message);
  g_free (message);
}

static void
on_scale_changed (GtkRange *range, gpointer user_data)
{
  GtkLabel *label = GTK_LABEL (user_data);
  double value = gtk_range_get_value (range);
  char *message = g_strdup_printf ("Scale value: %.2f", value);
  gtk_label_set_text (label, message);
  g_free (message);
}

static void
activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *main_box;
  GtkWidget *notebook;

  /* Create main window */
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "GTK4 Comprehensive Rendering Demo");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

  /* Main container */
  main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_window_set_child (GTK_WINDOW (window), main_box);

  /* Header */
  GtkWidget *header = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (header),
    "<span size='x-large' weight='bold'>GTK4 WebAssembly Rendering Demo</span>\n"
    "<span size='small'>Showcasing Widgets, Text, Images, and Cairo Graphics</span>");
  gtk_box_append (GTK_BOX (main_box), header);

  /* Notebook for different demo pages */
  notebook = gtk_notebook_new ();
  gtk_box_append (GTK_BOX (main_box), notebook);
  gtk_widget_set_vexpand (notebook, TRUE);

  /* ===== PAGE 1: Widgets Demo ===== */
  GtkWidget *widgets_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start (widgets_page, 10);
  gtk_widget_set_margin_end (widgets_page, 10);
  gtk_widget_set_margin_top (widgets_page, 10);
  gtk_widget_set_margin_bottom (widgets_page, 10);

  GtkWidget *status_label = gtk_label_new ("Click widgets to see status here");
  gtk_box_append (GTK_BOX (widgets_page), status_label);

  /* Button row */
  GtkWidget *button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append (GTK_BOX (widgets_page), button_box);

  const char *button_labels[] = {"Primary", "Secondary", "Success", "Danger", "Warning"};
  const char *button_styles[] = {"suggested-action", NULL, "success", "destructive-action", "warning"};

  for (int i = 0; i < 5; i++) {
    GtkWidget *btn = gtk_button_new_with_label (button_labels[i]);
    if (button_styles[i])
      gtk_widget_add_css_class (btn, button_styles[i]);
    g_signal_connect (btn, "clicked", G_CALLBACK (on_button_clicked), status_label);
    gtk_box_append (GTK_BOX (button_box), btn);
  }

  /* Entry field */
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (entry), "Type something and press Enter...");
  g_signal_connect (entry, "activate", G_CALLBACK (on_entry_activated), status_label);
  gtk_box_append (GTK_BOX (widgets_page), entry);

  /* Scale/Slider */
  GtkWidget *scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_range_set_value (GTK_RANGE (scale), 50);
  g_signal_connect (scale, "value-changed", G_CALLBACK (on_scale_changed), status_label);
  gtk_box_append (GTK_BOX (widgets_page), scale);

  /* Switches */
  GtkWidget *switch_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  for (int i = 0; i < 3; i++) {
    GtkWidget *sw = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (sw), i % 2 == 0);
    char *label_text = g_strdup_printf ("Switch %d", i + 1);
    GtkWidget *label = gtk_label_new (label_text);
    g_free (label_text);
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append (GTK_BOX (box), label);
    gtk_box_append (GTK_BOX (box), sw);
    gtk_box_append (GTK_BOX (switch_box), box);
  }
  gtk_box_append (GTK_BOX (widgets_page), switch_box);

  /* Progress bar */
  GtkWidget *progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.65);
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress), TRUE);
  gtk_box_append (GTK_BOX (widgets_page), progress);

  /* Spinner */
  GtkWidget *spinner = gtk_spinner_new ();
  gtk_spinner_start (GTK_SPINNER (spinner));
  gtk_box_append (GTK_BOX (widgets_page), spinner);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), widgets_page,
                           gtk_label_new ("Widgets"));

  /* ===== PAGE 2: Text Rendering Demo ===== */
  GtkWidget *text_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start (text_page, 10);
  gtk_widget_set_margin_end (text_page, 10);
  gtk_widget_set_margin_top (text_page, 10);
  gtk_widget_set_margin_bottom (text_page, 10);

  /* Text view with formatted content */
  GtkWidget *text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  gtk_text_buffer_insert_at_cursor (buffer,
    "GTK Text Rendering with Pango\n\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "This demo showcases advanced text rendering capabilities:\n\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "• Multiple font families (Sans, Serif, Monospace)\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "• Font attributes: bold, italic, underline\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "• Complex text shaping with HarfBuzz\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "• Full Unicode support: 你好, مرحبا, שלום\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "• Right-to-left text support\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "• Ligatures and kerning (fi, fl, ff)\n\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "Sample text in different sizes:\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "Small • Normal • Large • Extra Large\n\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "Monospace code example:\n", -1);
  gtk_text_buffer_insert_at_cursor (buffer,
    "  int main() {\n    printf(\"Hello, WASM!\");\n    return 0;\n  }\n", -1);

  GtkWidget *text_scroll = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (text_scroll), text_view);
  gtk_widget_set_vexpand (text_scroll, TRUE);
  gtk_box_append (GTK_BOX (text_page), text_scroll);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), text_page,
                           gtk_label_new ("Text Rendering"));

  /* ===== PAGE 3: Cairo Graphics Demo ===== */
  GtkWidget *graphics_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start (graphics_page, 10);
  gtk_widget_set_margin_end (graphics_page, 10);
  gtk_widget_set_margin_top (graphics_page, 10);
  gtk_widget_set_margin_bottom (graphics_page, 10);

  GtkWidget *graphics_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (graphics_label),
    "<b>Cairo 2D Graphics Rendering</b>\n"
    "Demonstrating shapes, gradients, text, and transformations");
  gtk_box_append (GTK_BOX (graphics_page), graphics_label);

  /* Drawing area for Cairo graphics */
  GtkWidget *drawing_area = gtk_drawing_area_new ();
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (drawing_area), DRAWING_AREA_WIDTH);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (drawing_area), DRAWING_AREA_HEIGHT);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (drawing_area), draw_function, NULL, NULL);

  GtkWidget *frame = gtk_frame_new (NULL);
  gtk_frame_set_child (GTK_FRAME (frame), drawing_area);
  gtk_widget_set_halign (frame, GTK_ALIGN_CENTER);
  gtk_box_append (GTK_BOX (graphics_page), frame);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), graphics_page,
                           gtk_label_new ("Cairo Graphics"));

  /* ===== PAGE 4: Layout Demo ===== */
  GtkWidget *layout_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start (layout_page, 10);
  gtk_widget_set_margin_end (layout_page, 10);
  gtk_widget_set_margin_top (layout_page, 10);
  gtk_widget_set_margin_bottom (layout_page, 10);

  gtk_box_append (GTK_BOX (layout_page),
                  gtk_label_new ("Grid Layout Manager Demo"));

  /* Grid layout */
  GtkWidget *grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 5);

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 4; col++) {
      char *label_text = g_strdup_printf ("Cell %d,%d", row, col);
      GtkWidget *btn = gtk_button_new_with_label (label_text);
      g_free (label_text);
      gtk_grid_attach (GTK_GRID (grid), btn, col, row, 1, 1);
    }
  }

  gtk_box_append (GTK_BOX (layout_page), grid);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), layout_page,
                           gtk_label_new ("Layouts"));

  /* Footer */
  GtkWidget *footer = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (footer),
    "<small>Powered by GTK4, Cairo, Pango, GdkPixbuf • Compiled to WebAssembly with Emscripten</small>");
  gtk_box_append (GTK_BOX (main_box), footer);

  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.discere.gtk.comprehensive-demo", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
