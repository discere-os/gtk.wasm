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

#include "gskgpubufferprivate.h"
#include "gskwebgpudeviceprivate.h"

#include <webgpu/webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_BUFFER (gsk_webgpu_buffer_get_type ())

G_DECLARE_FINAL_TYPE (GskWebGPUBuffer, gsk_webgpu_buffer, GSK, WEBGPU_BUFFER, GskGpuBuffer)

GskGpuBuffer *  gsk_webgpu_buffer_new        (GskWebGPUDevice   *device,
                                              gsize              size,
                                              GskGpuBufferUsage  usage);

WGPUBuffer      gsk_webgpu_buffer_get_buffer (GskWebGPUBuffer   *self);
gsize           gsk_webgpu_buffer_get_size   (GskWebGPUBuffer   *self);

G_END_DECLS