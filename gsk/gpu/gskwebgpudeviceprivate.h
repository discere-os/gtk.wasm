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

#include "gskgpudeviceprivate.h"

#include <webgpu/webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_DEVICE (gsk_webgpu_device_get_type ())
#define GSK_TYPE_WEBGPU_DAMAGE_TRACKER (gsk_webgpu_damage_tracker_get_type ())

G_DECLARE_FINAL_TYPE (GskWebGPUDevice, gsk_webgpu_device, GSK, WEBGPU_DEVICE, GskGpuDevice)
G_DECLARE_FINAL_TYPE (GskWebGPUDamageTracker, gsk_webgpu_damage_tracker, GSK, WEBGPU_DAMAGE_TRACKER, GObject)

/* Performance metrics structure following D3D12 patterns */
typedef struct {
  gdouble frame_time_ms;
  gdouble fps;
  guint32 draw_call_count;
  guint32 triangle_count;
  gsize allocated_memory;
  gsize peak_memory;
  guint32 resource_count;
} GskWebGPUPerformanceMetrics;

/* Damage tracking structure */
typedef struct {
  gfloat x, y, width, height;
} GskWebGPUDamageRect;

/* Device creation and management */
GskGpuDevice *          gsk_webgpu_device_new                    (WGPUDevice            device,
                                                                  WGPUQueue             queue,
                                                                  WGPUAdapter           adapter);

void                    gsk_webgpu_device_detect_features        (GskWebGPUDevice      *self);

WGPUDevice              gsk_webgpu_device_get_device             (GskWebGPUDevice      *self);
WGPUQueue               gsk_webgpu_device_get_queue              (GskWebGPUDevice      *self);
WGPUAdapter             gsk_webgpu_device_get_adapter            (GskWebGPUDevice      *self);

/* Pipeline cache management following D3D12 patterns */
WGPURenderPipeline      gsk_webgpu_device_get_cached_pipeline    (GskWebGPUDevice      *self,
                                                                  const gchar          *vertex_shader,
                                                                  const gchar          *fragment_shader,
                                                                  WGPUTextureFormat     color_format,
                                                                  WGPUTextureFormat     depth_format,
                                                                  WGPUPrimitiveTopology topology);

void                    gsk_webgpu_device_cache_pipeline         (GskWebGPUDevice      *self,
                                                                  const gchar          *vertex_shader,
                                                                  const gchar          *fragment_shader,
                                                                  WGPUTextureFormat     color_format,
                                                                  WGPUTextureFormat     depth_format,
                                                                  WGPUPrimitiveTopology topology,
                                                                  WGPURenderPipeline    pipeline);

/* Damage tracking */
GskWebGPUDamageTracker * gsk_webgpu_device_get_damage_tracker   (GskWebGPUDevice      *self);

/* Performance monitoring */
void                    gsk_webgpu_device_update_performance_counters (GskWebGPUDevice *self,
                                                                        guint32          draw_calls,
                                                                        guint32          triangles);

void                    gsk_webgpu_device_reset_performance_counters  (GskWebGPUDevice *self);

GskWebGPUPerformanceMetrics gsk_webgpu_device_get_performance_metrics (GskWebGPUDevice *self);

/* Feature detection */
gboolean                gsk_webgpu_device_has_feature            (GskWebGPUDevice      *self,
                                                                  WGPUFeatureName       feature);

/* Damage Tracker API */
GskWebGPUDamageTracker * gsk_webgpu_damage_tracker_new          (void);

void                    gsk_webgpu_damage_tracker_add            (GskWebGPUDamageTracker *tracker,
                                                                  WGPUTexture             texture,
                                                                  GskWebGPUDamageRect    *damage);

gboolean                gsk_webgpu_damage_tracker_has_damage     (GskWebGPUDamageTracker *tracker,
                                                                  WGPUTexture             texture);

GskWebGPUDamageRect *   gsk_webgpu_damage_tracker_get_damage    (GskWebGPUDamageTracker *tracker,
                                                                  WGPUTexture             texture);

void                    gsk_webgpu_damage_tracker_clear          (GskWebGPUDamageTracker *tracker);

void                    gsk_webgpu_damage_tracker_rotate         (GskWebGPUDamageTracker *tracker);

G_END_DECLS