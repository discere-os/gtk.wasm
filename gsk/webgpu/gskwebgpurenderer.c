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
#include "gskwebgpurendererprivate.h"
#include "gskwebgpudevice.h"
#include "gskwebgpupipelines.h"
#include "gskwebgpushaders.h"

#include "gskrendererprivate.h"
#include "gskdebugprivate.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>

struct _GskWebGPURendererPrivate
{
  GskWebGPUDevice *device;
  GskWebGPUPipelines *pipelines;
  GskWebGPUFeatures features;
  
  WGPUDevice wgpu_device;
  WGPUQueue wgpu_queue;
  WGPUSurface wgpu_surface;
  WGPUSwapChain wgpu_swapchain;
  WGPUTextureFormat surface_format;
  
  /* Canvas context */
  const char *canvas_selector;
  
  /* Render state */
  WGPUCommandEncoder command_encoder;
  WGPURenderPassEncoder render_pass;
  
  /* Resource management */
  GHashTable *texture_cache;
  GHashTable *buffer_cache;
  
  /* Performance monitoring */
  GTimer *frame_timer;
  double last_frame_time;
  int frame_count;
  
  gboolean is_initialized;
  gboolean is_realized;
};

G_DEFINE_TYPE_WITH_PRIVATE (GskWebGPURenderer, gsk_webgpu_renderer, GSK_TYPE_RENDERER)

static gboolean gsk_webgpu_renderer_realize       (GskRenderer  *renderer,
                                                    GdkSurface   *surface,
                                                    GError      **error);
static void     gsk_webgpu_renderer_unrealize     (GskRenderer  *renderer);
static WGPUTexture gsk_webgpu_renderer_render_texture (GskRenderer           *renderer,
                                                        GskRenderNode         *root,
                                                        const graphene_rect_t *viewport);
static void     gsk_webgpu_renderer_render         (GskRenderer          *renderer,
                                                     GskRenderNode        *root,
                                                     const cairo_region_t *region);

/* JavaScript WebGPU initialization helpers */
EM_JS(WGPUDevice, webgpu_init_device, (const char* canvas_selector, int* success), {
  return new Promise(async (resolve) => {
    try {
      // Check WebGPU availability
      if (!navigator.gpu) {
        console.warn('WebGPU not supported');
        Module.setValue(success, 0, 'i32');
        resolve(0);
        return;
      }

      // Request adapter
      const adapter = await navigator.gpu.requestAdapter({
        powerPreference: 'high-performance',
        forceFallbackAdapter: false
      });
      
      if (!adapter) {
        console.warn('Failed to get WebGPU adapter');
        Module.setValue(success, 0, 'i32');
        resolve(0);
        return;
      }

      // Request device with features
      const requiredFeatures = [];
      const supportedFeatures = adapter.features;
      
      // Add optional features if supported
      if (supportedFeatures.has('depth-clip-control'))
        requiredFeatures.push('depth-clip-control');
      if (supportedFeatures.has('texture-compression-bc'))
        requiredFeatures.push('texture-compression-bc');
      if (supportedFeatures.has('texture-compression-etc2'))
        requiredFeatures.push('texture-compression-etc2');
      if (supportedFeatures.has('texture-compression-astc'))
        requiredFeatures.push('texture-compression-astc');

      const device = await adapter.requestDevice({
        requiredFeatures,
        requiredLimits: {
          maxTextureDimension2D: 4096,
          maxTextureArrayLayers: 256,
          maxBindGroups: 8,
          maxUniformBuffersPerShaderStage: 12,
          maxStorageBuffersPerShaderStage: 8
        }
      });

      if (!device) {
        console.warn('Failed to create WebGPU device');
        Module.setValue(success, 0, 'i32');
        resolve(0);
        return;
      }

      // Configure canvas context
      const canvas = document.querySelector(UTF8ToString(canvas_selector));
      if (!canvas) {
        console.warn('Canvas not found:', UTF8ToString(canvas_selector));
        Module.setValue(success, 0, 'i32');
        resolve(0);
        return;
      }

      const context = canvas.getContext('webgpu');
      if (!context) {
        console.warn('Failed to get WebGPU context');
        Module.setValue(success, 0, 'i32');
        resolve(0);
        return;
      }

      const preferredFormat = navigator.gpu.getPreferredCanvasFormat();
      context.configure({
        device,
        format: preferredFormat,
        alphaMode: 'premultiplied',
        usage: GPUTextureUsage.RENDER_ATTACHMENT
      });

      // Store globals for C++ access
      Module.webgpu = {
        adapter,
        device,
        context,
        format: preferredFormat,
        canvas
      };

      console.log('WebGPU initialized successfully');
      Module.setValue(success, 1, 'i32');
      resolve(1); // Return success flag
    } catch (error) {
      console.error('WebGPU initialization failed:', error);
      Module.setValue(success, 0, 'i32');
      resolve(0);
    }
  });
});

EM_JS(WGPUQueue, webgpu_get_queue, (), {
  return Module.webgpu ? Module.webgpu.device.queue : 0;
});

EM_JS(WGPUTexture, webgpu_get_current_texture, (), {
  if (!Module.webgpu || !Module.webgpu.context) return 0;
  return Module.webgpu.context.getCurrentTexture();
});

EM_JS(WGPUTextureFormat, webgpu_get_surface_format, (), {
  return Module.webgpu ? Module.webgpu.format : 0;
});

EM_JS(void, webgpu_present, (), {
  // WebGPU presents automatically, but we can add performance monitoring here
  if (Module.webgpu && Module.webgpu.canvas) {
    // Optional: Trigger any post-render callbacks
    Module.webgpu.canvas.dispatchEvent(new CustomEvent('webgpu-frame-presented'));
  }
});

static void
gsk_webgpu_renderer_init (GskWebGPURenderer *self)
{
  GskWebGPURendererPrivate *priv = gsk_webgpu_renderer_get_instance_private (self);
  
  self->priv = priv;
  
  priv->device = NULL;
  priv->pipelines = NULL;
  priv->features = GSK_WEBGPU_FEATURE_NONE;
  
  priv->wgpu_device = 0;
  priv->wgpu_queue = 0;
  priv->wgpu_surface = 0;
  priv->wgpu_swapchain = 0;
  priv->surface_format = WGPUTextureFormat_Undefined;
  
  priv->canvas_selector = "#gtk-canvas"; /* Default canvas */
  
  priv->command_encoder = 0;
  priv->render_pass = 0;
  
  priv->texture_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  priv->buffer_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  
  priv->frame_timer = g_timer_new ();
  priv->last_frame_time = 0.0;
  priv->frame_count = 0;
  
  priv->is_initialized = FALSE;
  priv->is_realized = FALSE;
}

static void
gsk_webgpu_renderer_dispose (GObject *object)
{
  GskWebGPURenderer *self = GSK_WEBGPU_RENDERER (object);
  GskWebGPURendererPrivate *priv = self->priv;

  if (priv->is_realized)
    gsk_webgpu_renderer_unrealize (GSK_RENDERER (self));

  g_clear_object (&priv->device);
  g_clear_object (&priv->pipelines);
  
  g_clear_pointer (&priv->texture_cache, g_hash_table_unref);
  g_clear_pointer (&priv->buffer_cache, g_hash_table_unref);
  g_clear_pointer (&priv->frame_timer, g_timer_destroy);

  G_OBJECT_CLASS (gsk_webgpu_renderer_parent_class)->dispose (object);
}

static void
gsk_webgpu_renderer_class_init (GskWebGPURendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GskRendererClass *renderer_class = GSK_RENDERER_CLASS (klass);

  object_class->dispose = gsk_webgpu_renderer_dispose;

  renderer_class->realize = gsk_webgpu_renderer_realize;
  renderer_class->unrealize = gsk_webgpu_renderer_unrealize;
  renderer_class->render_texture = gsk_webgpu_renderer_render_texture;
  renderer_class->render = gsk_webgpu_renderer_render;
}

/**
 * gsk_webgpu_renderer_new:
 *
 * Creates a new WebGPU renderer.
 *
 * Returns: a new WebGPU renderer
 */
GskRenderer *
gsk_webgpu_renderer_new (void)
{
  return g_object_new (GSK_TYPE_WEBGPU_RENDERER, NULL);
}

/**
 * gsk_webgpu_renderer_is_available:
 *
 * Checks if WebGPU is available in the current browser environment.
 *
 * Returns: %TRUE if WebGPU is supported, %FALSE otherwise
 */
gboolean
gsk_webgpu_renderer_is_available (void)
{
  /* Check if WebGPU is available in the browser */
  return EM_ASM_INT({
    return (typeof navigator !== 'undefined' && 
            navigator.gpu !== undefined) ? 1 : 0;
  });
}

static gboolean
gsk_webgpu_renderer_realize (GskRenderer  *renderer,
                             GdkSurface   *surface,
                             GError      **error)
{
  GskWebGPURenderer *self = GSK_WEBGPU_RENDERER (renderer);
  GskWebGPURendererPrivate *priv = self->priv;
  int success = 0;
  
  if (priv->is_realized)
    return TRUE;

  /* Check WebGPU availability */
  if (!gsk_webgpu_renderer_is_available ())
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                   "WebGPU is not supported in this browser");
      return FALSE;
    }

  /* Initialize WebGPU device */
  WGPUDevice device = webgpu_init_device (priv->canvas_selector, &success);
  if (!success || !device)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to initialize WebGPU device");
      return FALSE;
    }

  priv->wgpu_device = device;
  priv->wgpu_queue = webgpu_get_queue ();
  priv->surface_format = webgpu_get_surface_format ();

  /* Create WebGPU wrapper objects */
  priv->device = gsk_webgpu_device_new (priv->wgpu_device, priv->wgpu_queue);
  priv->pipelines = gsk_webgpu_pipelines_new (priv->device);

  /* Auto-detect features */
  priv->features = GSK_WEBGPU_FEATURE_NONE;
  
#ifdef EMSCRIPTEN_SIMD
  priv->features |= GSK_WEBGPU_FEATURE_SIMD;
#endif

  /* Check for SharedArrayBuffer support (threading) */
  if (EM_ASM_INT({ return typeof SharedArrayBuffer !== 'undefined' ? 1 : 0; }))
    priv->features |= GSK_WEBGPU_FEATURE_THREADING;

  /* Initialize render pipelines */
  if (!gsk_webgpu_pipelines_init (priv->pipelines, priv->surface_format, error))
    return FALSE;

  priv->is_realized = TRUE;
  priv->is_initialized = TRUE;

  GSK_RENDERER_NOTE (renderer, RENDERER,
                     "WebGPU renderer realized with features: 0x%x", priv->features);

  return TRUE;
}

static void
gsk_webgpu_renderer_unrealize (GskRenderer *renderer)
{
  GskWebGPURenderer *self = GSK_WEBGPU_RENDERER (renderer);
  GskWebGPURendererPrivate *priv = self->priv;

  if (!priv->is_realized)
    return;

  /* Clean up WebGPU resources */
  g_clear_object (&priv->pipelines);
  g_clear_object (&priv->device);

  /* Clear caches */
  g_hash_table_remove_all (priv->texture_cache);
  g_hash_table_remove_all (priv->buffer_cache);

  priv->wgpu_device = 0;
  priv->wgpu_queue = 0;
  priv->wgpu_surface = 0;
  priv->wgpu_swapchain = 0;

  priv->is_realized = FALSE;

  GSK_RENDERER_NOTE (renderer, RENDERER, "WebGPU renderer unrealized");
}

static WGPUTexture
gsk_webgpu_renderer_render_texture (GskRenderer           *renderer,
                                     GskRenderNode         *root,
                                     const graphene_rect_t *viewport)
{
  GskWebGPURenderer *self = GSK_WEBGPU_RENDERER (renderer);
  GskWebGPURendererPrivate *priv = self->priv;
  
  if (!priv->is_realized)
    return 0;

  /* For now, return current swap chain texture */
  /* TODO: Implement offscreen rendering to texture */
  return webgpu_get_current_texture ();
}

static void
gsk_webgpu_renderer_render (GskRenderer          *renderer,
                            GskRenderNode        *root,
                            const cairo_region_t *region)
{
  GskWebGPURenderer *self = GSK_WEBGPU_RENDERER (renderer);
  GskWebGPURendererPrivate *priv = self->priv;
  
  if (!priv->is_realized || !root)
    return;

  /* Start frame timing */
  g_timer_start (priv->frame_timer);

  /* Get current swap chain texture */
  WGPUTexture surface_texture = webgpu_get_current_texture ();
  if (!surface_texture)
    return;

  /* Create command encoder */
  WGPUCommandEncoderDescriptor encoder_desc = {
    .label = "GTK Frame Command Encoder"
  };
  priv->command_encoder = wgpuDeviceCreateCommandEncoder (priv->wgpu_device, &encoder_desc);

  /* Begin render pass */
  WGPUTextureView surface_view = wgpuTextureCreateView (surface_texture, NULL);
  
  WGPURenderPassColorAttachment color_attachment = {
    .view = surface_view,
    .loadOp = WGPULoadOp_Clear,
    .storeOp = WGPUStoreOp_Store,
    .clearValue = { 1.0f, 1.0f, 1.0f, 1.0f } /* White background */
  };
  
  WGPURenderPassDescriptor render_pass_desc = {
    .label = "GTK Main Render Pass",
    .colorAttachmentCount = 1,
    .colorAttachments = &color_attachment
  };
  
  priv->render_pass = wgpuCommandEncoderBeginRenderPass (priv->command_encoder, &render_pass_desc);

  /* Render the scene graph */
  gsk_webgpu_renderer_render_node (self, root);

  /* End render pass */
  wgpuRenderPassEncoderEnd (priv->render_pass);
  wgpuRenderPassEncoderRelease (priv->render_pass);
  priv->render_pass = 0;

  /* Submit commands */
  WGPUCommandBufferDescriptor cmd_buffer_desc = {
    .label = "GTK Frame Commands"
  };
  WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish (priv->command_encoder, &cmd_buffer_desc);
  
  wgpuQueueSubmit (priv->wgpu_queue, 1, &cmd_buffer);

  /* Cleanup */
  wgpuCommandBufferRelease (cmd_buffer);
  wgpuCommandEncoderRelease (priv->command_encoder);
  wgpuTextureViewRelease (surface_view);
  priv->command_encoder = 0;

  /* Present frame */
  webgpu_present ();

  /* Update performance metrics */
  priv->last_frame_time = g_timer_elapsed (priv->frame_timer, NULL);
  priv->frame_count++;

  if (GSK_RENDERER_DEBUG_CHECK (renderer, RENDERER))
    {
      if (priv->frame_count % 60 == 0) /* Log every 60 frames */
        {
          GSK_RENDERER_NOTE (renderer, RENDERER,
                           "WebGPU: Frame %d rendered in %.2fms",
                           priv->frame_count, priv->last_frame_time * 1000.0);
        }
    }
}

/**
 * gsk_webgpu_renderer_get_features:
 * @renderer: a WebGPU renderer
 *
 * Gets the features enabled on the WebGPU renderer.
 *
 * Returns: the enabled features
 */
GskWebGPUFeatures
gsk_webgpu_renderer_get_features (GskWebGPURenderer *renderer)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_RENDERER (renderer), GSK_WEBGPU_FEATURE_NONE);
  
  return renderer->priv->features;
}

/**
 * gsk_webgpu_renderer_set_features:
 * @renderer: a WebGPU renderer  
 * @features: the features to enable
 *
 * Sets the features to enable on the WebGPU renderer.
 * This must be called before the renderer is realized.
 */
void
gsk_webgpu_renderer_set_features (GskWebGPURenderer *renderer,
                                   GskWebGPUFeatures  features)
{
  g_return_if_fail (GSK_IS_WEBGPU_RENDERER (renderer));
  g_return_if_fail (!renderer->priv->is_realized);
  
  renderer->priv->features = features;
}

/**
 * gsk_webgpu_renderer_get_device:
 * @renderer: a WebGPU renderer
 *
 * Gets the WebGPU device used by the renderer.
 *
 * Returns: (transfer none): the WebGPU device
 */
WGPUDevice
gsk_webgpu_renderer_get_device (GskWebGPURenderer *renderer)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_RENDERER (renderer), 0);
  
  return renderer->priv->wgpu_device;
}

/**
 * gsk_webgpu_renderer_get_queue:
 * @renderer: a WebGPU renderer
 *
 * Gets the WebGPU queue used by the renderer.
 *
 * Returns: (transfer none): the WebGPU queue
 */
WGPUQueue
gsk_webgpu_renderer_get_queue (GskWebGPURenderer *renderer)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_RENDERER (renderer), 0);
  
  return renderer->priv->wgpu_queue;
}

/**
 * gsk_webgpu_renderer_get_info:
 * @renderer: a WebGPU renderer
 *
 * Gets information about the WebGPU renderer.
 *
 * Returns: (transfer none): renderer information string
 */
const char *
gsk_webgpu_renderer_get_info (GskWebGPURenderer *renderer)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_RENDERER (renderer), NULL);
  
  if (!renderer->priv->is_realized)
    return "WebGPU Renderer (not realized)";
  
  return EM_ASM_INT({
    if (!Module.webgpu) return 0;
    
    const info = `WebGPU Renderer
Device: ${Module.webgpu.adapter ? 'Hardware-accelerated' : 'Software fallback'}
Format: ${Module.webgpu.format}
Features: ${Module.webgpu.device.features ? Array.from(Module.webgpu.device.features).join(', ') : 'None'}`;
    
    const len = lengthBytesUTF8(info) + 1;
    const ptr = _malloc(len);
    stringToUTF8(info, ptr, len);
    return ptr;
  });
}