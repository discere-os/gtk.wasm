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

#include "gskwebgpubufferprivate.h"
#include "gskwebgpudeviceprivate.h"

#include <webgpu/webgpu.h>

struct _GskWebGPUBuffer
{
  GskGpuBuffer parent_instance;

  WGPUBuffer buffer;
  gsize size;
  GskGpuBufferUsage usage;
  gpointer mapped_data;
  gboolean is_mapped;
};

G_DEFINE_FINAL_TYPE (GskWebGPUBuffer, gsk_webgpu_buffer, GSK_TYPE_GPU_BUFFER)

static void
gsk_webgpu_buffer_finalize (GObject *object)
{
  GskWebGPUBuffer *self = GSK_WEBGPU_BUFFER (object);

  if (self->is_mapped)
    {
      wgpuBufferUnmap (self->buffer);
      self->is_mapped = FALSE;
    }

  if (self->buffer)
    {
      wgpuBufferRelease (self->buffer);
      self->buffer = 0;
    }

  G_OBJECT_CLASS (gsk_webgpu_buffer_parent_class)->finalize (object);
}

static gpointer
gsk_webgpu_buffer_map (GskGpuBuffer *buffer)
{
  GskWebGPUBuffer *self = GSK_WEBGPU_BUFFER (buffer);

  if (self->is_mapped)
    return self->mapped_data;

  /* WebGPU async mapping - for now use synchronous approach */
  /* In a real implementation, this would need proper async handling */
  g_warning ("WebGPU buffer mapping not yet implemented - using fallback");

  return NULL;
}

static void
gsk_webgpu_buffer_unmap (GskGpuBuffer *buffer)
{
  GskWebGPUBuffer *self = GSK_WEBGPU_BUFFER (buffer);

  if (!self->is_mapped)
    return;

  wgpuBufferUnmap (self->buffer);
  self->is_mapped = FALSE;
  self->mapped_data = NULL;
}

static void
gsk_webgpu_buffer_class_init (GskWebGPUBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GskGpuBufferClass *buffer_class = GSK_GPU_BUFFER_CLASS (klass);

  object_class->finalize = gsk_webgpu_buffer_finalize;

  buffer_class->map = gsk_webgpu_buffer_map;
  buffer_class->unmap = gsk_webgpu_buffer_unmap;
}

static void
gsk_webgpu_buffer_init (GskWebGPUBuffer *self)
{
  self->buffer = 0;
  self->size = 0;
  self->usage = 0;
  self->mapped_data = NULL;
  self->is_mapped = FALSE;
}

static WGPUBufferUsage
gsk_gpu_buffer_usage_to_webgpu (GskGpuBufferUsage usage)
{
  WGPUBufferUsage webgpu_usage = WGPUBufferUsage_None;

  if (usage & GSK_GPU_BUFFER_USAGE_VERTEX)
    webgpu_usage |= WGPUBufferUsage_Vertex;
  if (usage & GSK_GPU_BUFFER_USAGE_INDEX)
    webgpu_usage |= WGPUBufferUsage_Index;
  if (usage & GSK_GPU_BUFFER_USAGE_UNIFORM)
    webgpu_usage |= WGPUBufferUsage_Uniform;
  if (usage & GSK_GPU_BUFFER_USAGE_STORAGE)
    webgpu_usage |= WGPUBufferUsage_Storage;

  /* Always allow copy operations for data updates */
  webgpu_usage |= WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;

  return webgpu_usage;
}

GskGpuBuffer *
gsk_webgpu_buffer_new (GskWebGPUDevice   *device,
                       gsize              size,
                       GskGpuBufferUsage  usage)
{
  GskWebGPUBuffer *self;
  WGPUBufferUsage webgpu_usage;
  WGPUBufferDescriptor buffer_desc;

  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), NULL);
  g_return_val_if_fail (size > 0, NULL);

  webgpu_usage = gsk_gpu_buffer_usage_to_webgpu (usage);

  buffer_desc = (WGPUBufferDescriptor) {
    .label = "GskWebGPUBuffer",
    .usage = webgpu_usage,
    .size = size,
    .mappedAtCreation = FALSE
  };

  self = g_object_new (GSK_TYPE_WEBGPU_BUFFER, NULL);
  self->size = size;
  self->usage = usage;
  self->buffer = wgpuDeviceCreateBuffer (gsk_webgpu_device_get_device (device),
                                         &buffer_desc);

  if (!self->buffer)
    {
      g_object_unref (self);
      return NULL;
    }

  return GSK_GPU_BUFFER (self);
}

WGPUBuffer
gsk_webgpu_buffer_get_buffer (GskWebGPUBuffer *self)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_BUFFER (self), 0);

  return self->buffer;
}

gsize
gsk_webgpu_buffer_get_size (GskWebGPUBuffer *self)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_BUFFER (self), 0);

  return self->size;
}