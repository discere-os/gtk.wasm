/*
 * Copyright © 2025 Superstruct Ltd, New Zealand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "gskwebgpurenderer.h"
#include "gskrendererprivate.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/**
 * GTK WebGPU Integration Layer
 * 
 * This file provides the integration points between GTK and the WebGPU renderer,
 * including automatic renderer selection and fallback handling.
 */

/* Global WebGPU availability flag */
static gboolean webgpu_available = FALSE;
static gboolean webgpu_checked = FALSE;

/**
 * gsk_webgpu_integration_init:
 *
 * Initializes WebGPU integration and checks availability.
 * Called during GTK initialization.
 */
void
gsk_webgpu_integration_init (void)
{
  if (webgpu_checked)
    return;

  webgpu_available = gsk_webgpu_renderer_is_available ();
  webgpu_checked = TRUE;

  if (webgpu_available)
    {
      g_info ("WebGPU renderer available and enabled");
    }
  else
    {
      g_info ("WebGPU not available, using fallback renderer");
    }
}

/**
 * gsk_webgpu_integration_is_available:
 *
 * Checks if WebGPU is available for use.
 *
 * Returns: %TRUE if WebGPU is available
 */
gboolean
gsk_webgpu_integration_is_available (void)
{
  if (!webgpu_checked)
    gsk_webgpu_integration_init ();

  return webgpu_available;
}

/**
 * gsk_webgpu_integration_create_renderer:
 * @surface: the surface to render to
 *
 * Creates the appropriate renderer for the given surface.
 * Prefers WebGPU but falls back to Broadway if unavailable.
 *
 * Returns: (transfer full): a new renderer
 */
GskRenderer *
gsk_webgpu_integration_create_renderer (GdkSurface *surface)
{
  GskRenderer *renderer = NULL;

  /* Try WebGPU first if available */
  if (gsk_webgpu_integration_is_available ())
    {
      renderer = gsk_webgpu_renderer_new ();
      
      /* Test realization to make sure WebGPU actually works */
      GError *error = NULL;
      if (!gsk_renderer_realize (renderer, surface, &error))
        {
          g_warning ("WebGPU renderer failed to realize: %s", 
                     error ? error->message : "Unknown error");
          g_clear_error (&error);
          g_clear_object (&renderer);
        }
    }

  /* Fallback to Broadway renderer */
  if (!renderer)
    {
      g_info ("Using Broadway renderer as fallback");
      
      /* Use Broadway renderer for WASM compatibility */
#ifdef GDK_WINDOWING_BROADWAY  
      renderer = gsk_broadway_renderer_new ();
#else
      /* If Broadway is not available, use Cairo renderer */
      renderer = gsk_cairo_renderer_new ();
#endif
    }

  return renderer;
}

/**
 * gsk_webgpu_integration_configure_renderer:
 * @renderer: the renderer to configure
 * @features: features to enable
 *
 * Configures a WebGPU renderer with optimal settings.
 */
void
gsk_webgpu_integration_configure_renderer (GskRenderer       *renderer,
                                            GskWebGPUFeatures  features)
{
  if (!GSK_IS_WEBGPU_RENDERER (renderer))
    return;

  GskWebGPURenderer *webgpu_renderer = GSK_WEBGPU_RENDERER (renderer);

  /* Auto-detect optimal features if not specified */
  if (features == GSK_WEBGPU_FEATURE_NONE)
    {
      features = gsk_webgpu_renderer_get_features (webgpu_renderer);
      
      /* Enable additional features based on system capabilities */
      
      /* Check for high-DPI displays - enable higher quality rendering */
      GdkSurface *surface = gsk_renderer_get_surface (renderer);
      if (surface)
        {
          int scale_factor = gdk_surface_get_scale_factor (surface);
          if (scale_factor > 1)
            {
              features |= GSK_WEBGPU_FEATURE_DEPTH_STENCIL;
            }
        }

      /* Enable compute shaders for blur effects if supported */
      features |= GSK_WEBGPU_FEATURE_COMPUTE_SHADERS;
    }

  gsk_webgpu_renderer_set_features (webgpu_renderer, features);
}

/* Environment variable configuration */
static GskWebGPUFeatures
parse_webgpu_features_from_env (void)
{
  const char *env_features = g_getenv ("GSK_WEBGPU_FEATURES");
  GskWebGPUFeatures features = GSK_WEBGPU_FEATURE_NONE;

  if (!env_features)
    return features;

  /* Parse comma-separated feature list */
  gchar **feature_list = g_strsplit (env_features, ",", -1);
  
  for (gchar **feature = feature_list; *feature; feature++)
    {
      gchar *trimmed = g_strstrip (*feature);
      
      if (g_strcmp0 (trimmed, "simd") == 0)
        features |= GSK_WEBGPU_FEATURE_SIMD;
      else if (g_strcmp0 (trimmed, "threading") == 0)
        features |= GSK_WEBGPU_FEATURE_THREADING;
      else if (g_strcmp0 (trimmed, "compute") == 0)
        features |= GSK_WEBGPU_FEATURE_COMPUTE_SHADERS;
      else if (g_strcmp0 (trimmed, "bindless") == 0)
        features |= GSK_WEBGPU_FEATURE_BINDLESS_TEXTURES;
      else if (g_strcmp0 (trimmed, "depth") == 0)
        features |= GSK_WEBGPU_FEATURE_DEPTH_STENCIL;
      else if (g_strcmp0 (trimmed, "all") == 0)
        features = GSK_WEBGPU_FEATURE_SIMD | GSK_WEBGPU_FEATURE_THREADING |
                   GSK_WEBGPU_FEATURE_COMPUTE_SHADERS | GSK_WEBGPU_FEATURE_BINDLESS_TEXTURES |
                   GSK_WEBGPU_FEATURE_DEPTH_STENCIL;
      else
        g_warning ("Unknown WebGPU feature: %s", trimmed);
    }

  g_strfreev (feature_list);
  return features;
}

/**
 * gsk_webgpu_integration_get_optimal_features:
 *
 * Determines the optimal WebGPU features for the current environment.
 *
 * Returns: optimal feature set
 */
GskWebGPUFeatures
gsk_webgpu_integration_get_optimal_features (void)
{
  /* Check environment variable first */
  GskWebGPUFeatures env_features = parse_webgpu_features_from_env ();
  if (env_features != GSK_WEBGPU_FEATURE_NONE)
    return env_features;

  /* Auto-detect based on system capabilities */
  GskWebGPUFeatures features = GSK_WEBGPU_FEATURE_NONE;

  /* Always enable SIMD if available */
#ifdef EMSCRIPTEN_SIMD
  features |= GSK_WEBGPU_FEATURE_SIMD;
#endif

  /* Enable threading if SharedArrayBuffer is available */
  if (EM_ASM_INT({ return typeof SharedArrayBuffer !== 'undefined' ? 1 : 0; }))
    features |= GSK_WEBGPU_FEATURE_THREADING;

  /* Enable compute shaders for modern browsers */
  features |= GSK_WEBGPU_FEATURE_COMPUTE_SHADERS;

  /* Enable depth/stencil for advanced rendering */
  features |= GSK_WEBGPU_FEATURE_DEPTH_STENCIL;

  return features;
}

/**
 * gsk_webgpu_integration_setup_canvas:
 * @canvas_selector: CSS selector for the canvas element
 *
 * Sets up the HTML5 canvas for WebGPU rendering.
 * This should be called before creating the renderer.
 */
void
gsk_webgpu_integration_setup_canvas (const char *canvas_selector)
{
  EM_ASM({
    const canvas = document.querySelector(UTF8ToString($0));
    if (!canvas) {
      console.warn('Canvas not found:', UTF8ToString($0));
      return;
    }
    
    // Configure canvas for WebGPU
    canvas.style.display = 'block';
    
    // Set up proper sizing
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * devicePixelRatio;
    canvas.height = rect.height * devicePixelRatio;
    
    // Store canvas reference for WebGPU initialization
    Module.gtkCanvas = canvas;
    
    console.log('GTK Canvas configured:', canvas.width + 'x' + canvas.height);
  }, canvas_selector);
}

/**
 * gsk_webgpu_integration_log_renderer_info:
 * @renderer: the renderer to log information about
 *
 * Logs detailed information about the active renderer.
 */
void
gsk_webgpu_integration_log_renderer_info (GskRenderer *renderer)
{
  if (!renderer)
    return;

  if (GSK_IS_WEBGPU_RENDERER (renderer))
    {
      GskWebGPURenderer *webgpu_renderer = GSK_WEBGPU_RENDERER (renderer);
      const char *info = gsk_webgpu_renderer_get_info (webgpu_renderer);
      GskWebGPUFeatures features = gsk_webgpu_renderer_get_features (webgpu_renderer);
      
      g_info ("WebGPU Renderer Active:");
      g_info ("  %s", info ? info : "No additional info");
      g_info ("  Features: 0x%08x", features);
      
      if (features & GSK_WEBGPU_FEATURE_SIMD)
        g_info ("    - SIMD acceleration enabled");
      if (features & GSK_WEBGPU_FEATURE_THREADING)
        g_info ("    - Multi-threading support");
      if (features & GSK_WEBGPU_FEATURE_COMPUTE_SHADERS)
        g_info ("    - Compute shaders available");
      if (features & GSK_WEBGPU_FEATURE_BINDLESS_TEXTURES)
        g_info ("    - Bindless textures supported");
      if (features & GSK_WEBGPU_FEATURE_DEPTH_STENCIL)
        g_info ("    - Advanced depth/stencil operations");
    }
  else
    {
      g_info ("Fallback Renderer Active: %s", G_OBJECT_TYPE_NAME (renderer));
    }
}