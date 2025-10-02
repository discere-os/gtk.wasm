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

#include <glib-object.h>
#include <webgpu/webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_DEVICE (gsk_webgpu_device_get_type ())

G_DECLARE_FINAL_TYPE (GskWebGPUDevice, gsk_webgpu_device, GSK, WEBGPU_DEVICE, GObject)

/**
 * GskWebGPUDevice:
 *
 * Wrapper object for WebGPU device and queue management.
 * Provides high-level resource creation and management.
 */

GskWebGPUDevice *gsk_webgpu_device_new                (WGPUDevice  device,
                                                        WGPUQueue   queue);

WGPUDevice       gsk_webgpu_device_get_device         (GskWebGPUDevice *device);
WGPUQueue        gsk_webgpu_device_get_queue          (GskWebGPUDevice *device);

/* Resource creation helpers */
WGPUBuffer       gsk_webgpu_device_create_buffer      (GskWebGPUDevice *device,
                                                        const char      *label,
                                                        size_t           size,
                                                        WGPUBufferUsage  usage);

WGPUTexture      gsk_webgpu_device_create_texture     (GskWebGPUDevice *device,
                                                        const char      *label,
                                                        uint32_t         width,
                                                        uint32_t         height,
                                                        WGPUTextureFormat format,
                                                        WGPUTextureUsage usage);

WGPUSampler      gsk_webgpu_device_create_sampler     (GskWebGPUDevice *device,
                                                        const char      *label,
                                                        WGPUAddressMode  address_mode,
                                                        WGPUFilterMode   filter_mode);

WGPUBindGroup    gsk_webgpu_device_create_bind_group  (GskWebGPUDevice       *device,
                                                        const char            *label,
                                                        WGPUBindGroupLayout    layout,
                                                        uint32_t               entry_count,
                                                        const WGPUBindGroupEntry *entries);

/* Memory management */
void             gsk_webgpu_device_write_buffer       (GskWebGPUDevice *device,
                                                        WGPUBuffer       buffer,
                                                        size_t           offset,
                                                        const void      *data,
                                                        size_t           size);

void             gsk_webgpu_device_write_texture      (GskWebGPUDevice *device,
                                                        WGPUTexture      texture,
                                                        const void      *data,
                                                        size_t           data_size,
                                                        uint32_t         width,
                                                        uint32_t         height,
                                                        WGPUTextureFormat format);

/* Performance monitoring */
double           gsk_webgpu_device_get_memory_usage   (GskWebGPUDevice *device);
uint32_t         gsk_webgpu_device_get_resource_count (GskWebGPUDevice *device);

G_END_DECLS