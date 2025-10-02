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

#include "gskgpuframeprivate.h"
#include "gskwebgpudeviceprivate.h"

#include <webgpu/webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_FRAME (gsk_webgpu_frame_get_type ())

G_DECLARE_FINAL_TYPE (GskWebGPUFrame, gsk_webgpu_frame, GSK, WEBGPU_FRAME, GskGpuFrame)

/* Resource state enum for state tracking */
typedef enum {
  GSK_WEBGPU_RESOURCE_STATE_UNDEFINED = 0,
  GSK_WEBGPU_RESOURCE_STATE_VERTEX_BUFFER = 1,
  GSK_WEBGPU_RESOURCE_STATE_INDEX_BUFFER = 2,
  GSK_WEBGPU_RESOURCE_STATE_RENDER_TARGET = 4,
  GSK_WEBGPU_RESOURCE_STATE_SHADER_RESOURCE = 8,
  GSK_WEBGPU_RESOURCE_STATE_COPY_SOURCE = 16,
  GSK_WEBGPU_RESOURCE_STATE_COPY_DEST = 32,
  GSK_WEBGPU_RESOURCE_STATE_PRESENT = 64
} GskWebGPUResourceState;

/* Frame creation */
GskGpuFrame *           gsk_webgpu_frame_new                    (GskWebGPUDevice        *device,
                                                                 GskGpuImage            *image);

/* Resource state management following D3D12 patterns */
void                    gsk_webgpu_frame_transition_resource    (GskWebGPUFrame         *self,
                                                                 WGPUTexture             texture,
                                                                 GskWebGPUResourceState  new_state);

/* Command recording */
void                    gsk_webgpu_frame_set_pipeline           (GskWebGPUFrame         *self,
                                                                 WGPURenderPipeline      pipeline);

void                    gsk_webgpu_frame_set_bind_group         (GskWebGPUFrame         *self,
                                                                 guint32                 index,
                                                                 WGPUBindGroup           bind_group);

void                    gsk_webgpu_frame_set_vertex_buffer      (GskWebGPUFrame         *self,
                                                                 guint32                 slot,
                                                                 WGPUBuffer              buffer,
                                                                 guint64                 offset,
                                                                 guint64                 size);

void                    gsk_webgpu_frame_set_index_buffer       (GskWebGPUFrame         *self,
                                                                 WGPUBuffer              buffer,
                                                                 WGPUIndexFormat         format,
                                                                 guint64                 offset,
                                                                 guint64                 size);

void                    gsk_webgpu_frame_draw                   (GskWebGPUFrame         *self,
                                                                 guint32                 vertex_count,
                                                                 guint32                 instance_count,
                                                                 guint32                 first_vertex,
                                                                 guint32                 first_instance);

void                    gsk_webgpu_frame_draw_indexed           (GskWebGPUFrame         *self,
                                                                 guint32                 index_count,
                                                                 guint32                 instance_count,
                                                                 guint32                 first_index,
                                                                 gint32                  base_vertex,
                                                                 guint32                 first_instance);

/* Performance monitoring */
GskWebGPUPerformanceMetrics gsk_webgpu_frame_get_performance_metrics (GskWebGPUFrame *self);

G_END_DECLS