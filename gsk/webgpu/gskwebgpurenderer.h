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

#pragma once

#include <gdk/gdk.h>
#include <gsk/gskrenderer.h>
#include <emscripten/emscripten.h>
#include <webgpu/webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_RENDERER (gsk_webgpu_renderer_get_type ())

#define GSK_WEBGPU_RENDERER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSK_TYPE_WEBGPU_RENDERER, GskWebGPURenderer))
#define GSK_IS_WEBGPU_RENDERER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSK_TYPE_WEBGPU_RENDERER))
#define GSK_WEBGPU_RENDERER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GSK_TYPE_WEBGPU_RENDERER, GskWebGPURendererClass))
#define GSK_IS_WEBGPU_RENDERER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GSK_TYPE_WEBGPU_RENDERER))
#define GSK_WEBGPU_RENDERER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GSK_TYPE_WEBGPU_RENDERER, GskWebGPURendererClass))

typedef struct _GskWebGPURenderer                GskWebGPURenderer;
typedef struct _GskWebGPURendererClass           GskWebGPURendererClass;
typedef struct _GskWebGPURendererPrivate        GskWebGPURendererPrivate;

/**
 * GskWebGPURenderer:
 *
 * A GSK renderer that uses WebGPU for hardware-accelerated rendering.
 * 
 * This renderer provides high-performance 2D and compute capabilities
 * by leveraging WebGPU's modern graphics API, with automatic fallback
 * to software rendering when WebGPU is unavailable.
 */
struct _GskWebGPURenderer
{
  GskRenderer parent_instance;

  GskWebGPURendererPrivate *priv;
};

struct _GskWebGPURendererClass
{
  GskRendererClass parent_class;
};

/**
 * GskWebGPUFeatures:
 * @GSK_WEBGPU_FEATURE_NONE: No special features
 * @GSK_WEBGPU_FEATURE_SIMD: WASM SIMD instructions available
 * @GSK_WEBGPU_FEATURE_THREADING: SharedArrayBuffer and threading support
 * @GSK_WEBGPU_FEATURE_COMPUTE_SHADERS: Compute shader support for image processing
 * @GSK_WEBGPU_FEATURE_BINDLESS_TEXTURES: Bindless texture arrays (if supported)
 * @GSK_WEBGPU_FEATURE_DEPTH_STENCIL: Advanced depth/stencil operations
 *
 * Features that can be enabled on the WebGPU renderer.
 */
typedef enum {
  GSK_WEBGPU_FEATURE_NONE                = 0,
  GSK_WEBGPU_FEATURE_SIMD               = 1 << 0,
  GSK_WEBGPU_FEATURE_THREADING          = 1 << 1,
  GSK_WEBGPU_FEATURE_COMPUTE_SHADERS    = 1 << 2,
  GSK_WEBGPU_FEATURE_BINDLESS_TEXTURES  = 1 << 3,
  GSK_WEBGPU_FEATURE_DEPTH_STENCIL      = 1 << 4,
} GskWebGPUFeatures;

GDK_AVAILABLE_IN_ALL
GType                   gsk_webgpu_renderer_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GskRenderer *           gsk_webgpu_renderer_new               (void);

GDK_AVAILABLE_IN_ALL
gboolean                gsk_webgpu_renderer_is_available      (void);

GDK_AVAILABLE_IN_ALL
GskWebGPUFeatures       gsk_webgpu_renderer_get_features      (GskWebGPURenderer *renderer);

GDK_AVAILABLE_IN_ALL
void                    gsk_webgpu_renderer_set_features      (GskWebGPURenderer *renderer,
                                                               GskWebGPUFeatures  features);

GDK_AVAILABLE_IN_ALL
WGPUDevice              gsk_webgpu_renderer_get_device        (GskWebGPURenderer *renderer);

GDK_AVAILABLE_IN_ALL
WGPUQueue               gsk_webgpu_renderer_get_queue         (GskWebGPURenderer *renderer);

GDK_AVAILABLE_IN_ALL
const char *            gsk_webgpu_renderer_get_info          (GskWebGPURenderer *renderer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GskWebGPURenderer, g_object_unref)

G_END_DECLS