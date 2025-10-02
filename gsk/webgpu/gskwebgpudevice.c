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

#include "gskwebgpudevice.h"
#include <webgpu/webgpu.h>
#include <stdbool.h>

struct _GskWebGPUDevice
{
  GObject parent_instance;

  WGPUDevice device;
  WGPUQueue queue;
  
  /* Resource tracking */
  guint32 resource_count;
  GHashTable *resource_registry;
  
  /* Memory monitoring */
  gsize allocated_memory;
  gsize peak_memory;
};

G_DEFINE_FINAL_TYPE (GskWebGPUDevice, gsk_webgpu_device, G_TYPE_OBJECT)

static void
gsk_webgpu_device_dispose (GObject *object)
{
  GskWebGPUDevice *self = GSK_WEBGPU_DEVICE (object);

  g_clear_pointer (&self->resource_registry, g_hash_table_unref);

  if (self->device)
    {
      wgpuDeviceRelease (self->device);
      self->device = 0;
    }

  if (self->queue)
    {
      wgpuQueueRelease (self->queue);
      self->queue = 0;
    }

  G_OBJECT_CLASS (gsk_webgpu_device_parent_class)->dispose (object);
}

static void
gsk_webgpu_device_class_init (GskWebGPUDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gsk_webgpu_device_dispose;
}

static void
gsk_webgpu_device_init (GskWebGPUDevice *self)
{
  self->device = 0;
  self->queue = 0;
  self->resource_count = 0;
  self->allocated_memory = 0;
  self->peak_memory = 0;
  
  self->resource_registry = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                   NULL, g_free);
}

/**
 * gsk_webgpu_device_new:
 * @device: WebGPU device handle
 * @queue: WebGPU queue handle
 *
 * Creates a new WebGPU device wrapper.
 *
 * Returns: a new #GskWebGPUDevice
 */
GskWebGPUDevice *
gsk_webgpu_device_new (WGPUDevice device,
                        WGPUQueue  queue)
{
  GskWebGPUDevice *self = g_object_new (GSK_TYPE_WEBGPU_DEVICE, NULL);
  
  self->device = device;
  self->queue = queue;
  
  return self;
}

/**
 * gsk_webgpu_device_get_device:
 * @device: a #GskWebGPUDevice
 *
 * Gets the underlying WebGPU device handle.
 *
 * Returns: the WebGPU device
 */
WGPUDevice
gsk_webgpu_device_get_device (GskWebGPUDevice *device)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);
  
  return device->device;
}

/**
 * gsk_webgpu_device_get_queue:
 * @device: a #GskWebGPUDevice
 *
 * Gets the underlying WebGPU queue handle.
 *
 * Returns: the WebGPU queue
 */
WGPUQueue
gsk_webgpu_device_get_queue (GskWebGPUDevice *device)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);
  
  return device->queue;
}

/**
 * gsk_webgpu_device_create_buffer:
 * @device: a #GskWebGPUDevice
 * @label: debug label for the buffer
 * @size: size of the buffer in bytes
 * @usage: buffer usage flags
 *
 * Creates a WebGPU buffer with the specified parameters.
 *
 * Returns: a new WebGPU buffer
 */
WGPUBuffer
gsk_webgpu_device_create_buffer (GskWebGPUDevice *device,
                                  const char      *label,
                                  size_t           size,
                                  WGPUBufferUsage  usage)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);
  g_return_val_if_fail (size > 0, 0);

  WGPUBufferDescriptor buffer_desc = {
    .label = label,
    .usage = usage,
    .size = size,
    .mappedAtCreation = false
  };

  WGPUBuffer buffer = wgpuDeviceCreateBuffer (device->device, &buffer_desc);
  
  if (buffer)
    {
      device->resource_count++;
      device->allocated_memory += size;
      
      if (device->allocated_memory > device->peak_memory)
        device->peak_memory = device->allocated_memory;
        
      /* Track resource for debugging */
      gchar *debug_info = g_strdup_printf ("Buffer: %s (%zu bytes)", 
                                           label ? label : "unnamed", size);
      g_hash_table_insert (device->resource_registry, 
                           GSIZE_TO_POINTER ((gsize)buffer), debug_info);
    }

  return buffer;
}

/**
 * gsk_webgpu_device_create_texture:
 * @device: a #GskWebGPUDevice
 * @label: debug label for the texture
 * @width: texture width
 * @height: texture height
 * @format: texture format
 * @usage: texture usage flags
 *
 * Creates a 2D WebGPU texture.
 *
 * Returns: a new WebGPU texture
 */
WGPUTexture
gsk_webgpu_device_create_texture (GskWebGPUDevice   *device,
                                   const char        *label,
                                   uint32_t           width,
                                   uint32_t           height,
                                   WGPUTextureFormat  format,
                                   WGPUTextureUsage   usage)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);
  g_return_val_if_fail (width > 0 && height > 0, 0);

  WGPUTextureDescriptor texture_desc = {
    .label = label,
    .usage = usage,
    .dimension = WGPUTextureDimension_2D,
    .size = { width, height, 1 },
    .format = format,
    .mipLevelCount = 1,
    .sampleCount = 1
  };

  WGPUTexture texture = wgpuDeviceCreateTexture (device->device, &texture_desc);
  
  if (texture)
    {
      device->resource_count++;
      
      /* Estimate texture memory usage */
      gsize bytes_per_pixel = 4; /* Assume RGBA8 for now */
      gsize texture_size = width * height * bytes_per_pixel;
      device->allocated_memory += texture_size;
      
      if (device->allocated_memory > device->peak_memory)
        device->peak_memory = device->allocated_memory;
        
      /* Track resource */
      gchar *debug_info = g_strdup_printf ("Texture: %s (%ux%u, %zu bytes)",
                                           label ? label : "unnamed", 
                                           width, height, texture_size);
      g_hash_table_insert (device->resource_registry,
                           GSIZE_TO_POINTER ((gsize)texture), debug_info);
    }

  return texture;
}

/**
 * gsk_webgpu_device_create_sampler:
 * @device: a #GskWebGPUDevice
 * @label: debug label for the sampler
 * @address_mode: address mode for texture sampling
 * @filter_mode: filter mode for texture sampling
 *
 * Creates a WebGPU sampler.
 *
 * Returns: a new WebGPU sampler
 */
WGPUSampler
gsk_webgpu_device_create_sampler (GskWebGPUDevice *device,
                                   const char      *label,
                                   WGPUAddressMode  address_mode,
                                   WGPUFilterMode   filter_mode)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);

  WGPUSamplerDescriptor sampler_desc = {
    .label = label,
    .addressModeU = address_mode,
    .addressModeV = address_mode,
    .addressModeW = address_mode,
    .magFilter = filter_mode,
    .minFilter = filter_mode,
    .mipmapFilter = WGPUMipmapFilterMode_Linear,
    .lodMinClamp = 0.0f,
    .lodMaxClamp = 32.0f,
    .compare = WGPUCompareFunction_Undefined,
    .maxAnisotropy = 1
  };

  WGPUSampler sampler = wgpuDeviceCreateSampler (device->device, &sampler_desc);
  
  if (sampler)
    {
      device->resource_count++;
      
      gchar *debug_info = g_strdup_printf ("Sampler: %s", 
                                           label ? label : "unnamed");
      g_hash_table_insert (device->resource_registry,
                           GSIZE_TO_POINTER ((gsize)sampler), debug_info);
    }

  return sampler;
}

/**
 * gsk_webgpu_device_create_bind_group:
 * @device: a #GskWebGPUDevice
 * @label: debug label for the bind group
 * @layout: bind group layout
 * @entry_count: number of entries
 * @entries: array of bind group entries
 *
 * Creates a WebGPU bind group.
 *
 * Returns: a new WebGPU bind group
 */
WGPUBindGroup
gsk_webgpu_device_create_bind_group (GskWebGPUDevice            *device,
                                      const char                 *label,
                                      WGPUBindGroupLayout         layout,
                                      uint32_t                    entry_count,
                                      const WGPUBindGroupEntry   *entries)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);
  g_return_val_if_fail (layout != 0, 0);
  g_return_val_if_fail (entries != NULL || entry_count == 0, 0);

  WGPUBindGroupDescriptor bind_group_desc = {
    .label = label,
    .layout = layout,
    .entryCount = entry_count,
    .entries = entries
  };

  WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup (device->device, &bind_group_desc);
  
  if (bind_group)
    {
      device->resource_count++;
      
      gchar *debug_info = g_strdup_printf ("BindGroup: %s (%u entries)",
                                           label ? label : "unnamed", entry_count);
      g_hash_table_insert (device->resource_registry,
                           GSIZE_TO_POINTER ((gsize)bind_group), debug_info);
    }

  return bind_group;
}

/**
 * gsk_webgpu_device_write_buffer:
 * @device: a #GskWebGPUDevice
 * @buffer: WebGPU buffer to write to
 * @offset: offset in bytes
 * @data: data to write
 * @size: size of data in bytes
 *
 * Writes data to a WebGPU buffer.
 */
void
gsk_webgpu_device_write_buffer (GskWebGPUDevice *device,
                                 WGPUBuffer       buffer,
                                 size_t           offset,
                                 const void      *data,
                                 size_t           size)
{
  g_return_if_fail (GSK_IS_WEBGPU_DEVICE (device));
  g_return_if_fail (buffer != 0);
  g_return_if_fail (data != NULL);
  g_return_if_fail (size > 0);

  wgpuQueueWriteBuffer (device->queue, buffer, offset, data, size);
}

/**
 * gsk_webgpu_device_write_texture:
 * @device: a #GskWebGPUDevice
 * @texture: WebGPU texture to write to
 * @data: texture data
 * @data_size: size of texture data
 * @width: texture width
 * @height: texture height
 * @format: texture format
 *
 * Writes image data to a WebGPU texture.
 */
void
gsk_webgpu_device_write_texture (GskWebGPUDevice   *device,
                                  WGPUTexture        texture,
                                  const void        *data,
                                  size_t             data_size,
                                  uint32_t           width,
                                  uint32_t           height,
                                  WGPUTextureFormat  format)
{
  g_return_if_fail (GSK_IS_WEBGPU_DEVICE (device));
  g_return_if_fail (texture != 0);
  g_return_if_fail (data != NULL);
  g_return_if_fail (data_size > 0);
  g_return_if_fail (width > 0 && height > 0);

  WGPUTexelCopyTextureInfo destination = {
    .texture = texture,
    .mipLevel = 0,
    .origin = { 0, 0, 0 },
    .aspect = WGPUTextureAspect_All
  };

  WGPUTexelCopyBufferLayout data_layout = {
    .offset = 0,
    .bytesPerRow = width * 4, /* Assume RGBA8 */
    .rowsPerImage = height
  };

  WGPUExtent3D write_size = { width, height, 1 };

  wgpuQueueWriteTexture (device->queue, &destination, data, data_size,
                         &data_layout, &write_size);
}

/**
 * gsk_webgpu_device_get_memory_usage:
 * @device: a #GskWebGPUDevice
 *
 * Gets the current estimated memory usage.
 *
 * Returns: memory usage in bytes
 */
double
gsk_webgpu_device_get_memory_usage (GskWebGPUDevice *device)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0.0);
  
  return (double)device->allocated_memory;
}

/**
 * gsk_webgpu_device_get_resource_count:
 * @device: a #GskWebGPUDevice
 *
 * Gets the current number of allocated resources.
 *
 * Returns: resource count
 */
uint32_t
gsk_webgpu_device_get_resource_count (GskWebGPUDevice *device)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), 0);
  
  return device->resource_count;
}