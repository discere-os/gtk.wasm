/*
 * GTK WebGPU Real Demo Application
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

#include <gtk/gtk.h>
#include <emscripten/emscripten.h>
#include <webgpu/webgpu.h>

#ifdef GSK_WEBGPU_ENABLED
#include <gsk/webgpu/gskwebgpurenderer.h>
#endif

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *canvas_container;
    GskRenderer *renderer;
    int widget_count;
    GList *widgets;
} GTKWebGPUApp;

static GTKWebGPUApp *app_instance = NULL;

// Forward declarations
static void activate_app(GtkApplication *app, gpointer user_data);
static void setup_webgpu_rendering(GTKWebGPUApp *app);
static GtkWidget* create_main_window(GTKWebGPUApp *app);

// JavaScript interface functions
EMSCRIPTEN_KEEPALIVE
int gtk_webgpu_init(void) {
    if (app_instance) {
        return 1; // Already initialized
    }
    
    app_instance = g_new0(GTKWebGPUApp, 1);
    app_instance->widget_count = 0;
    app_instance->widgets = NULL;
    
    // Create GTK application
    app_instance->app = gtk_application_new("com.superstruct.gtk-webgpu-demo",
                                           G_APPLICATION_DEFAULT_FLAGS);
    
    g_signal_connect(app_instance->app, "activate", G_CALLBACK(activate_app), app_instance);
    
    printf("[GTK WebGPU] Application initialized\n");
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int gtk_webgpu_run(void) {
    if (!app_instance) {
        printf("[GTK WebGPU Error] Application not initialized\n");
        return 0;
    }
    
    // Run the GTK application (non-blocking for Emscripten)
    g_application_run(G_APPLICATION(app_instance->app), 0, NULL);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int gtk_webgpu_create_window(void) {
    if (!app_instance) return 0;
    
    if (!app_instance->window) {
        app_instance->window = create_main_window(app_instance);
        gtk_window_present(GTK_WINDOW(app_instance->window));
        printf("[GTK WebGPU] Window created and presented\n");
    }
    
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int gtk_webgpu_add_widget(const char *widget_type) {
    if (!app_instance || !app_instance->window) {
        printf("[GTK WebGPU Error] No window available for widget\n");
        return 0;
    }
    
    GtkWidget *widget = NULL;
    
    if (g_strcmp0(widget_type, "button") == 0) {
        widget = gtk_button_new_with_label(g_strdup_printf("Button %d", app_instance->widget_count + 1));
    } else if (g_strcmp0(widget_type, "label") == 0) {
        widget = gtk_label_new(g_strdup_printf("Label %d", app_instance->widget_count + 1));
    } else if (g_strcmp0(widget_type, "entry") == 0) {
        widget = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(widget), 
                                     g_strdup_printf("Entry %d", app_instance->widget_count + 1));
    } else {
        printf("[GTK WebGPU Error] Unknown widget type: %s\n", widget_type);
        return 0;
    }
    
    if (widget) {
        // Add widget to container
        if (app_instance->canvas_container) {
            if (GTK_IS_BOX(app_instance->canvas_container)) {
                gtk_box_append(GTK_BOX(app_instance->canvas_container), widget);
            } else {
                gtk_container_add(GTK_CONTAINER(app_instance->canvas_container), widget);
            }
        }
        
        app_instance->widgets = g_list_append(app_instance->widgets, widget);
        app_instance->widget_count++;
        
        printf("[GTK WebGPU] Added %s widget (total: %d)\n", widget_type, app_instance->widget_count);
        return 1;
    }
    
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int gtk_webgpu_get_widget_count(void) {
    return app_instance ? app_instance->widget_count : 0;
}

EMSCRIPTEN_KEEPALIVE
int gtk_webgpu_clear_widgets(void) {
    if (!app_instance) return 0;
    
    // Remove all widgets from container
    if (app_instance->canvas_container) {
        GList *iter = app_instance->widgets;
        while (iter) {
            GtkWidget *widget = GTK_WIDGET(iter->data);
            if (widget) {
                gtk_container_remove(GTK_CONTAINER(app_instance->canvas_container), widget);
            }
            iter = g_list_next(iter);
        }
    }
    
    g_list_free(app_instance->widgets);
    app_instance->widgets = NULL;
    app_instance->widget_count = 0;
    
    printf("[GTK WebGPU] All widgets cleared\n");
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* gtk_webgpu_get_renderer_info(void) {
    if (!app_instance || !app_instance->renderer) {
        return "No renderer available";
    }
    
#ifdef GSK_WEBGPU_ENABLED
    if (GSK_IS_WEBGPU_RENDERER(app_instance->renderer)) {
        return "WebGPU Hardware Accelerated";
    }
#endif
    
    if (GSK_IS_GL_RENDERER(app_instance->renderer)) {
        return "OpenGL Hardware Accelerated";
    }
    
    if (GSK_IS_CAIRO_RENDERER(app_instance->renderer)) {
        return "Cairo Software Rendering";
    }
    
    return "Unknown Renderer";
}

static void activate_app(GtkApplication *app, gpointer user_data) {
    GTKWebGPUApp *demo_app = (GTKWebGPUApp*)user_data;
    
    printf("[GTK WebGPU] Application activated\n");
    
    // Setup WebGPU rendering
    setup_webgpu_rendering(demo_app);
    
    // The window will be created when requested via JavaScript
    printf("[GTK WebGPU] Ready for window creation\n");
}

static void setup_webgpu_rendering(GTKWebGPUApp *app) {
    GskRenderer *renderer = NULL;
    
#ifdef GSK_WEBGPU_ENABLED
    // Try to create WebGPU renderer first
    printf("[GTK WebGPU] Attempting to create WebGPU renderer...\n");
    renderer = gsk_webgpu_renderer_new();
    
    if (renderer) {
        printf("[GTK WebGPU] WebGPU renderer created successfully\n");
    }
#endif
    
    // Fallback to GL renderer if WebGPU not available
    if (!renderer) {
        printf("[GTK WebGPU] Falling back to GL renderer...\n");
        renderer = gsk_gl_renderer_new();
    }
    
    // Final fallback to Cairo renderer
    if (!renderer) {
        printf("[GTK WebGPU] Falling back to Cairo renderer...\n");
        renderer = gsk_cairo_renderer_new();
    }
    
    app->renderer = renderer;
    printf("[GTK WebGPU] Renderer setup complete: %s\n", gtk_webgpu_get_renderer_info());
}

static GtkWidget* create_main_window(GTKWebGPUApp *app) {
    GtkWidget *window = gtk_application_window_new(app->app);
    gtk_window_set_title(GTK_WINDOW(window), "GTK WebGPU Demo");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    // Create main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    
    // Add header label
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), 
                        "<big><b>GTK WebGPU Live Demo</b></big>\n"
                        "<small>Hardware-Accelerated UI Rendering</small>");
    gtk_label_set_justify(GTK_LABEL(header), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(vbox), header);
    
    // Create scrolled window for widgets
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 400);
    gtk_box_append(GTK_BOX(vbox), scrolled);
    
    // Create widget container (this will hold dynamically added widgets)
    app->canvas_container = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(app->canvas_container), 6);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(app->canvas_container), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(app->canvas_container), 10);
    gtk_widget_set_margin_start(app->canvas_container, 20);
    gtk_widget_set_margin_end(app->canvas_container, 20);
    gtk_widget_set_margin_top(app->canvas_container, 20);
    gtk_widget_set_margin_bottom(app->canvas_container, 20);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), app->canvas_container);
    
    // Add status bar
    GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(status_box, 20);
    gtk_widget_set_margin_end(status_box, 20);
    gtk_widget_set_margin_bottom(status_box, 10);
    gtk_box_append(GTK_BOX(vbox), status_box);
    
    GtkWidget *renderer_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(renderer_label), 
                        g_strdup_printf("<small>Renderer: %s</small>", 
                                      gtk_webgpu_get_renderer_info()));
    gtk_box_append(GTK_BOX(status_box), renderer_label);
    
    // Set custom renderer if available
    if (app->renderer) {
        GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
        if (surface) {
            gsk_renderer_realize(app->renderer, surface);
        }
    }
    
    return window;
}

// Cleanup function
EMSCRIPTEN_KEEPALIVE
void gtk_webgpu_cleanup(void) {
    if (app_instance) {
        if (app_instance->renderer) {
            gsk_renderer_unrealize(app_instance->renderer);
            g_object_unref(app_instance->renderer);
        }
        
        if (app_instance->widgets) {
            g_list_free(app_instance->widgets);
        }
        
        if (app_instance->app) {
            g_object_unref(app_instance->app);
        }
        
        g_free(app_instance);
        app_instance = NULL;
    }
    
    printf("[GTK WebGPU] Cleanup completed\n");
}

// Main function for Emscripten
int main(int argc, char *argv[]) {
    printf("[GTK WebGPU] Starting GTK WebGPU application\n");
    
    // Initialize GTK
    gtk_init();
    
    // The actual initialization will be called from JavaScript
    printf("[GTK WebGPU] GTK initialized, ready for JavaScript interface\n");
    
    return 0;
}