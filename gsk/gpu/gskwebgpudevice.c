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

#include "gskwebgpudeviceprivate.h"
#include "gskwebgpubufferprivate.h"
#include "gskwebgpuimageprivate.h"
#include "gskwebgpuframeprivate.h"

#include <webgpu/webgpu.h>
#include <emscripten/emscripten.h>

struct _GskWebGPUDevice
{
  GskGpuDevice parent_instance;

  WGPUDevice device;
  WGPUQueue queue;
  WGPUAdapter adapter;

  /* Pipeline cache following D3D12 patterns */
  GHashTable *pipeline_cache;
  GHashTable *bind_group_layout_cache;
  GHashTable *bind_group_cache;

  /* Resource tracking */
  guint32 resource_count;
  gsize allocated_memory;
  gsize peak_memory;

  /* Damage tracking for multi-buffering */
  GskWebGPUDamageTracker *damage_tracker;

  /* Capabilities */
  WGPUFeatureName *features;
  gsize feature_count;
  gboolean has_timestamp_query;
  gboolean has_texture_compression_bc;

  /* Performance metrics */
  gdouble last_frame_time;
  guint32 draw_call_count;
  guint32 triangle_count;
};

G_DEFINE_FINAL_TYPE (GskWebGPUDevice, gsk_webgpu_device, GSK_TYPE_GPU_DEVICE)

static gboolean
gsk_webgpu_device_supports_format (GskGpuDevice *device,
                                   GdkMemoryFormat format)
{
  /* WebGPU supports most formats that GTK uses */
  switch (format)
    {
    case GDK_MEMORY_A8:
    case GDK_MEMORY_A8B8G8R8_PREMULTIPLIED:
    case GDK_MEMORY_A8B8G8R8:
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8A8:
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
    case GDK_MEMORY_B8G8R8A8:
    case GDK_MEMORY_R8G8B8:
    case GDK_MEMORY_B8G8R8:
      return TRUE;

    case GDK_MEMORY_R16G16B16:
    case GDK_MEMORY_R16G16B16A16_PREMULTIPLIED:
    case GDK_MEMORY_R16G16B16A16:
    case GDK_MEMORY_R16G16B16A16_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_R16G16B16A16_FLOAT:
    case GDK_MEMORY_R32G32B32_FLOAT:
    case GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_R32G32B32A32_FLOAT:
      /* These require additional feature checks */
      return FALSE;

    case GDK_MEMORY_G8A8_PREMULTIPLIED:
    case GDK_MEMORY_G8A8:
    case GDK_MEMORY_G8:
    case GDK_MEMORY_G16A16_PREMULTIPLIED:
    case GDK_MEMORY_G16A16:
    case GDK_MEMORY_G16:
    case GDK_MEMORY_A16:
    case GDK_MEMORY_A16_FLOAT:
    case GDK_MEMORY_A32_FLOAT:
      return TRUE;

    default:
      return FALSE;
    }
}

static GskGpuImage *
gsk_webgpu_device_create_offscreen_image (GskGpuDevice    *device,
                                          gboolean         with_mipmap,
                                          GdkMemoryFormat  format,
                                          gsize            width,
                                          gsize            height)
{
  GskWebGPUDevice *self = GSK_WEBGPU_DEVICE (device);
  WGPUTextureFormat webgpu_format;
  WGPUTextureUsage usage;

  /* Convert GTK format to WebGPU format */
  switch (format)
    {
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8A8:
      webgpu_format = WGPUTextureFormat_RGBA8Unorm;
      break;
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
    case GDK_MEMORY_B8G8R8A8:
      webgpu_format = WGPUTextureFormat_BGRA8Unorm;
      break;
    default:
      webgpu_format = WGPUTextureFormat_RGBA8Unorm;
      break;
    }

  usage = WGPUTextureUsage_RenderAttachment |
          WGPUTextureUsage_TextureBinding |
          WGPUTextureUsage_CopyDst |
          WGPUTextureUsage_CopySrc;

  return gsk_webgpu_image_new (self, width, height, webgpu_format, usage, with_mipmap);
}

static GskGpuImage *
gsk_webgpu_device_create_atlas_image (GskGpuDevice *device,
                                      gsize         width,
                                      gsize         height)
{
  return gsk_webgpu_device_create_offscreen_image (device,
                                                   FALSE,
                                                   GDK_MEMORY_R8G8B8A8_PREMULTIPLIED,
                                                   width,
                                                   height);
}

static GskGpuImage *
gsk_webgpu_device_create_upload_image (GskGpuDevice    *device,
                                       gboolean         with_mipmap,
                                       GdkMemoryFormat  format,
                                       gboolean         try_srgb,
                                       gsize            width,
                                       gsize            height)
{
  return gsk_webgpu_device_create_offscreen_image (device,
                                                   with_mipmap,
                                                   format,
                                                   width,
                                                   height);
}

static GskGpuBuffer *
gsk_webgpu_device_create_buffer (GskGpuDevice      *device,
                                 gsize              size,
                                 GskGpuBufferUsage  usage)
{
  GskWebGPUDevice *self = GSK_WEBGPU_DEVICE (device);

  return gsk_webgpu_buffer_new (self, size, usage);
}

static void
gsk_webgpu_device_finalize (GObject *object)
{
  GskWebGPUDevice *self = GSK_WEBGPU_DEVICE (object);

  g_clear_pointer (&self->pipeline_cache, g_hash_table_unref);
  g_clear_pointer (&self->bind_group_layout_cache, g_hash_table_unref);
  g_clear_pointer (&self->bind_group_cache, g_hash_table_unref);
  g_clear_object (&self->damage_tracker);

  g_free (self->features);

  if (self->queue)
    {
      wgpuQueueRelease (self->queue);
      self->queue = 0;
    }

  if (self->device)
    {
      wgpuDeviceRelease (self->device);
      self->device = 0;
    }

  if (self->adapter)
    {
      wgpuAdapterRelease (self->adapter);
      self->adapter = 0;
    }

  G_OBJECT_CLASS (gsk_webgpu_device_parent_class)->finalize (object);
}

static void
gsk_webgpu_device_class_init (GskWebGPUDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GskGpuDeviceClass *device_class = GSK_GPU_DEVICE_CLASS (klass);

  object_class->finalize = gsk_webgpu_device_finalize;

  device_class->supports_format = gsk_webgpu_device_supports_format;
  device_class->create_offscreen_image = gsk_webgpu_device_create_offscreen_image;
  device_class->create_atlas_image = gsk_webgpu_device_create_atlas_image;
  device_class->create_upload_image = gsk_webgpu_device_create_upload_image;
  device_class->create_buffer = gsk_webgpu_device_create_buffer;
}

static void
gsk_webgpu_device_init (GskWebGPUDevice *self)
{
  self->device = 0;
  self->queue = 0;
  self->adapter = 0;

  self->pipeline_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->bind_group_layout_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->bind_group_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  self->resource_count = 0;
  self->allocated_memory = 0;
  self->peak_memory = 0;

  self->damage_tracker = NULL; /* Created on demand */

  self->features = NULL;
  self->feature_count = 0;
  self->has_timestamp_query = FALSE;
  self->has_texture_compression_bc = FALSE;

  self->last_frame_time = 0.0;
  self->draw_call_count = 0;
  self->triangle_count = 0;
}

GskGpuDevice *
gsk_webgpu_device_new (WGPUDevice  device,
                       WGPUQueue   queue,
                       WGPUAdapter adapter)
{
  GskWebGPUDevice *self;

  g_return_val_if_fail (device != 0, NULL);
  g_return_val_if_fail (queue != 0, NULL);

  self = g_object_new (GSK_TYPE_WEBGPU_DEVICE, NULL);

  self->device = device;
  self->queue = queue;
  self->adapter = adapter;

  /* Detect features */
  gsk_webgpu_device_detect_features (self);

  /* Initialize damage tracker */
  self->damage_tracker = gsk_webgpu_damage_tracker_new ();

  return GSK_GPU_DEVICE (self);
}

void
gsk_webgpu_device_detect_features (GskWebGPUDevice *self)
{
  g_return_if_fail (GSK_IS_WEBGPU_DEVICE (self));

  if (!self->adapter)
    return;

  /* Get adapter features */
  gsize feature_count = wgpuAdapterEnumerateFeatures (self->adapter, NULL);
  if (feature_count > 0)
    {
      self->features = g_new (WGPUFeatureName, feature_count);
      self->feature_count = wgpuAdapterEnumerateFeatures (self->adapter, self->features);

      /* Check for specific features we care about */
      for (gsize i = 0; i < self->feature_count; i++)
        {
          switch (self->features[i])
            {
            case WGPUFeatureName_TimestampQuery:
              self->has_timestamp_query = TRUE;
              break;
            case WGPUFeatureName_TextureCompressionBC:
              self->has_texture_compression_bc = TRUE;
              break;
            default:
              break;
            }
        }
    }

  GSK_DEBUG (RENDERER, "WebGPU Features detected: timestamp-query=%s, bc-compression=%s",
             self->has_timestamp_query ? "yes" : "no",
             self->has_texture_compression_bc ? "yes" : "no");
}

WGPUDevice
gsk_webgpu_device_get_device (GskWebGPUDevice *self)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), 0);

  return self->device;
}

WGPUQueue
gsk_webgpu_device_get_queue (GskWebGPUDevice *self)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), 0);

  return self->queue;
}

WGPUAdapter
gsk_webgpu_device_get_adapter (GskWebGPUDevice *self)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), 0);

  return self->adapter;
}

/* Pipeline Cache Management following D3D12 patterns */
static gchar *
gsk_webgpu_device_compute_pipeline_key (const gchar           *vertex_shader,
                                        const gchar           *fragment_shader,
                                        WGPUTextureFormat      color_format,
                                        WGPUTextureFormat      depth_format,
                                        WGPUPrimitiveTopology  topology)
{
  return g_strdup_printf ("%s|%s|%d|%d|%d",
                         vertex_shader,
                         fragment_shader,
                         (int)color_format,
                         (int)depth_format,
                         (int)topology);
}

WGPURenderPipeline
gsk_webgpu_device_get_cached_pipeline (GskWebGPUDevice       *self,
                                       const gchar           *vertex_shader,
                                       const gchar           *fragment_shader,
                                       WGPUTextureFormat      color_format,
                                       WGPUTextureFormat      depth_format,
                                       WGPUPrimitiveTopology  topology)
{
  gchar *key;
  WGPURenderPipeline pipeline;

  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), 0);

  key = gsk_webgpu_device_compute_pipeline_key (vertex_shader, fragment_shader,
                                                color_format, depth_format, topology);

  pipeline = g_hash_table_lookup (self->pipeline_cache, key);
  g_free (key);

  return pipeline;
}

void
gsk_webgpu_device_cache_pipeline (GskWebGPUDevice       *self,
                                  const gchar           *vertex_shader,
                                  const gchar           *fragment_shader,
                                  WGPUTextureFormat      color_format,
                                  WGPUTextureFormat      depth_format,
                                  WGPUPrimitiveTopology  topology,
                                  WGPURenderPipeline     pipeline)
{
  gchar *key;

  g_return_if_fail (GSK_IS_WEBGPU_DEVICE (self));
  g_return_if_fail (pipeline != 0);

  key = gsk_webgpu_device_compute_pipeline_key (vertex_shader, fragment_shader,
                                                color_format, depth_format, topology);

  g_hash_table_insert (self->pipeline_cache, key, (gpointer)pipeline);
}

/* Damage Tracking following D3D12 patterns */
GskWebGPUDamageTracker *
gsk_webgpu_device_get_damage_tracker (GskWebGPUDevice *self)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), NULL);

  return self->damage_tracker;
}

/* Performance Monitoring */
void
gsk_webgpu_device_update_performance_counters (GskWebGPUDevice *self,
                                               guint32          draw_calls,
                                               guint32          triangles)
{
  g_return_if_fail (GSK_IS_WEBGPU_DEVICE (self));

  self->draw_call_count += draw_calls;
  self->triangle_count += triangles;
}

void
gsk_webgpu_device_reset_performance_counters (GskWebGPUDevice *self)
{
  g_return_if_fail (GSK_IS_WEBGPU_DEVICE (self));

  self->draw_call_count = 0;
  self->triangle_count = 0;
  self->last_frame_time = g_get_monotonic_time () / 1000.0; /* Convert to ms */
}

GskWebGPUPerformanceMetrics
gsk_webgpu_device_get_performance_metrics (GskWebGPUDevice *self)
{
  GskWebGPUPerformanceMetrics metrics = { 0 };
  gdouble current_time;

  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), metrics);

  current_time = g_get_monotonic_time () / 1000.0;

  metrics.frame_time_ms = current_time - self->last_frame_time;
  metrics.draw_call_count = self->draw_call_count;
  metrics.triangle_count = self->triangle_count;
  metrics.allocated_memory = self->allocated_memory;
  metrics.peak_memory = self->peak_memory;
  metrics.resource_count = self->resource_count;

  if (metrics.frame_time_ms > 0)
    metrics.fps = 1000.0 / metrics.frame_time_ms;
  else
    metrics.fps = 0.0;

  return metrics;
}

gboolean
gsk_webgpu_device_has_feature (GskWebGPUDevice *self,
                               WGPUFeatureName  feature)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (self), FALSE);

  for (gsize i = 0; i < self->feature_count; i++)
    {
      if (self->features[i] == feature)
        return TRUE;
    }

  return FALSE;
}