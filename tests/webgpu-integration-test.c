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
 */

#include <gtk/gtk.h>

#ifdef GSK_WEBGPU_ENABLED
#include <gsk/webgpu/gskwebgpurenderer.h>
#include <gsk/webgpu/gskwebgpuintegration.c>

static void
test_webgpu_availability (void)
{
  gboolean available = gsk_webgpu_integration_is_available ();
  g_test_message ("WebGPU availability: %s", available ? "YES" : "NO");
  
  /* On non-WebGPU platforms, this should return FALSE */
#ifdef __EMSCRIPTEN__
  /* On Emscripten, availability depends on browser support */
  g_test_message ("Running on Emscripten platform");
#else
  g_assert_false (available);
#endif
}

static void
test_webgpu_renderer_creation (void)
{
  GskRenderer *renderer = gsk_webgpu_renderer_new ();
  g_assert_true (GSK_IS_WEBGPU_RENDERER (renderer));
  g_object_unref (renderer);
}

static void
test_webgpu_integration_create_renderer (void)
{
  /* Create a mock surface for testing */
  GdkSurface *surface = NULL; /* In a real test, you'd create a proper surface */
  
  GskRenderer *renderer = gsk_webgpu_integration_create_renderer (surface);
  g_assert_nonnull (renderer);
  
  /* Verify we got some kind of renderer (WebGPU or fallback) */
  g_test_message ("Created renderer: %s", G_OBJECT_TYPE_NAME (renderer));
  
  g_object_unref (renderer);
}

static void
test_webgpu_features_parsing (void)
{
  /* Test environment variable parsing */
  g_setenv ("GSK_WEBGPU_FEATURES", "simd,threading,compute", TRUE);
  
  GskWebGPUFeatures features = gsk_webgpu_integration_get_optimal_features ();
  
  g_assert_true (features & GSK_WEBGPU_FEATURE_SIMD);
  g_assert_true (features & GSK_WEBGPU_FEATURE_THREADING);
  g_assert_true (features & GSK_WEBGPU_FEATURE_COMPUTE_SHADERS);
  
  g_unsetenv ("GSK_WEBGPU_FEATURES");
}

int
main (int argc, char *argv[])
{
  gtk_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/webgpu/availability", test_webgpu_availability);
  g_test_add_func ("/webgpu/renderer-creation", test_webgpu_renderer_creation);
  g_test_add_func ("/webgpu/integration-create-renderer", test_webgpu_integration_create_renderer);
  g_test_add_func ("/webgpu/features-parsing", test_webgpu_features_parsing);

  return g_test_run ();
}

#else

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_skip ("WebGPU support not compiled in");
  return 0;
}

#endif /* GSK_WEBGPU_ENABLED */