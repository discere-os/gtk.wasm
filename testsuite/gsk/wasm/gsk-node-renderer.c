/*
 * GSK Node Renderer for WASM
 * Renders GSK render nodes to Cairo surfaces for testing
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <gtk/gtk.h>
#include <gsk/gsk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum render dimensions
#define MAX_WIDTH 1000
#define MAX_HEIGHT 1000

// Global state
static cairo_surface_t *render_surface = NULL;
static cairo_t *render_cr = NULL;
static GskRenderer *renderer = NULL;
static GdkSurface *window = NULL;

// Error handling
typedef enum {
    TEST_SUCCESS = 0,
    TEST_ERROR_INIT = -1,
    TEST_ERROR_LOAD = -2,
    TEST_ERROR_RENDER = -3,
    TEST_ERROR_BOUNDS = -4,
} TestError;

// Deserialization error callback
static void
deserialize_error_func(const GskParseLocation *start,
                       const GskParseLocation *end,
                       const GError           *error,
                       gpointer                user_data)
{
    printf("Parse error at %zu:%zu", start->lines + 1, start->line_chars + 1);
    if (start->lines != end->lines || start->line_chars != end->line_chars)
    {
        printf("-%zu:%zu", end->lines + 1, end->line_chars + 1);
    }
    printf(": %s\n", error->message);
}

// Load render node from .node file
static GskRenderNode *
load_node_file(const char *node_file)
{
    GBytes *bytes;
    gsize len;
    char *contents;
    GError *error = NULL;
    GskRenderNode *node;

    if (!g_file_get_contents(node_file, &contents, &len, &error))
    {
        printf("Could not open node file '%s': %s\n", node_file, error->message);
        g_clear_error(&error);
        return NULL;
    }

    bytes = g_bytes_new_take(contents, len);
    node = gsk_render_node_deserialize(bytes, deserialize_error_func, NULL);
    g_bytes_unref(bytes);

    if (!node)
    {
        printf("Failed to deserialize node file '%s'\n", node_file);
        return NULL;
    }

    return node;
}

// Initialize renderer
EMSCRIPTEN_KEEPALIVE
int init_renderer(int width, int height)
{
    if (width <= 0 || width > MAX_WIDTH || height <= 0 || height > MAX_HEIGHT)
    {
        printf("Invalid dimensions: %dx%d (max %dx%d)\n",
               width, height, MAX_WIDTH, MAX_HEIGHT);
        return TEST_ERROR_INIT;
    }

    // Create Cairo surface for rendering
    render_surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, width, height);

    if (cairo_surface_status(render_surface) != CAIRO_STATUS_SUCCESS)
    {
        printf("Failed to create Cairo surface\n");
        return TEST_ERROR_INIT;
    }

    render_cr = cairo_create(render_surface);
    if (cairo_status(render_cr) != CAIRO_STATUS_SUCCESS)
    {
        printf("Failed to create Cairo context\n");
        cairo_surface_destroy(render_surface);
        return TEST_ERROR_INIT;
    }

    // Initialize GTK if not already done
    if (!gtk_is_initialized())
    {
        gtk_init();
    }

    // Create window surface for GSK renderer
    window = gdk_surface_new_toplevel(gdk_display_get_default());
    renderer = gsk_renderer_new_for_surface(window);

    if (!renderer)
    {
        printf("Failed to create GSK renderer\n");
        cairo_destroy(render_cr);
        cairo_surface_destroy(render_surface);
        return TEST_ERROR_INIT;
    }

    printf("Renderer initialized: %dx%d\n", width, height);
    return TEST_SUCCESS;
}

// Render node file to Cairo surface
EMSCRIPTEN_KEEPALIVE
int render_node_file(const char *node_path)
{
    GskRenderNode *node;
    GdkTexture *texture;
    graphene_rect_t bounds;
    int width, height;

    if (!renderer || !render_surface)
    {
        printf("Renderer not initialized\n");
        return TEST_ERROR_INIT;
    }

    // Load node
    node = load_node_file(node_path);
    if (!node)
    {
        return TEST_ERROR_LOAD;
    }

    // Get node bounds
    gsk_render_node_get_bounds(node, &bounds);
    width = (int)ceil(bounds.size.width);
    height = (int)ceil(bounds.size.height);

    printf("Rendering node: %s (%dx%d)\n", node_path, width, height);

    // Check bounds fit in surface
    int surf_width = cairo_image_surface_get_width(render_surface);
    int surf_height = cairo_image_surface_get_height(render_surface);

    if (width > surf_width || height > surf_height)
    {
        printf("Node bounds (%dx%d) exceed surface (%dx%d)\n",
               width, height, surf_width, surf_height);
        gsk_render_node_unref(node);
        return TEST_ERROR_BOUNDS;
    }

    // Clear surface
    cairo_save(render_cr);
    cairo_set_operator(render_cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(render_cr);
    cairo_restore(render_cr);

    // Render node to texture
    texture = gsk_renderer_render_texture(renderer, node, NULL);
    if (!texture)
    {
        printf("Failed to render node to texture\n");
        gsk_render_node_unref(node);
        return TEST_ERROR_RENDER;
    }

    // Download texture data and copy to Cairo surface
    gsize stride;
    guchar *data = gdk_texture_download(texture, &stride);

    if (data)
    {
        // Copy texture data to Cairo surface
        unsigned char *surface_data = cairo_image_surface_get_data(render_surface);
        int surface_stride = cairo_image_surface_get_stride(render_surface);

        for (int y = 0; y < height; y++)
        {
            memcpy(
                surface_data + y * surface_stride,
                data + y * stride,
                width * 4  // 4 bytes per pixel (RGBA)
            );
        }

        g_free(data);
        cairo_surface_mark_dirty(render_surface);
    }

    g_object_unref(texture);
    gsk_render_node_unref(node);

    printf("Render complete\n");
    return TEST_SUCCESS;
}

// Get pointer to Cairo surface data for JavaScript access
EMSCRIPTEN_KEEPALIVE
unsigned char* get_render_data()
{
    if (!render_surface)
    {
        printf("No render surface available\n");
        return NULL;
    }

    cairo_surface_flush(render_surface);
    return cairo_image_surface_get_data(render_surface);
}

// Get surface stride
EMSCRIPTEN_KEEPALIVE
int get_render_stride()
{
    if (!render_surface) return 0;
    return cairo_image_surface_get_stride(render_surface);
}

// Get surface dimensions
EMSCRIPTEN_KEEPALIVE
int get_render_width()
{
    if (!render_surface) return 0;
    return cairo_image_surface_get_width(render_surface);
}

EMSCRIPTEN_KEEPALIVE
int get_render_height()
{
    if (!render_surface) return 0;
    return cairo_image_surface_get_height(render_surface);
}

// Cleanup
EMSCRIPTEN_KEEPALIVE
void cleanup_renderer()
{
    if (renderer)
    {
        gsk_renderer_unrealize(renderer);
        g_object_unref(renderer);
        renderer = NULL;
    }

    if (window)
    {
        gdk_surface_destroy(window);
        window = NULL;
    }

    if (render_cr)
    {
        cairo_destroy(render_cr);
        render_cr = NULL;
    }

    if (render_surface)
    {
        cairo_surface_destroy(render_surface);
        render_surface = NULL;
    }

    printf("Renderer cleaned up\n");
}

// Main entry point (called by Emscripten)
int main(int argc, char **argv)
{
    printf("GSK Node Renderer WASM\n");
    printf("Ready to render .node files\n");
    return 0;
}
