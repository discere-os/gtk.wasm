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

#include "gskgpuimage.h"
#include "gskwebgpudevice.h"
#include <webgpu/webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_IMAGE (gsk_webgpu_image_get_type())
G_DECLARE_FINAL_TYPE(GskWebGPUImage, gsk_webgpu_image, GSK, WEBGPU_IMAGE, GskGpuImage)

/* Resource states for WebGPU texture state transitions */
typedef enum {
  GSK_WEBGPU_RESOURCE_STATE_UNDEFINED = 0,
  GSK_WEBGPU_RESOURCE_STATE_RENDER_TARGET,
  GSK_WEBGPU_RESOURCE_STATE_SHADER_RESOURCE,
  GSK_WEBGPU_RESOURCE_STATE_COPY_SOURCE,
  GSK_WEBGPU_RESOURCE_STATE_COPY_DEST,
  GSK_WEBGPU_RESOURCE_STATE_PRESENT
} GskWebGPUResourceState;

/* Damage tracking for optimized updates */
typedef struct {
  cairo_region_t *region;
  gboolean needs_full_update;
  gint64 last_update_time;
} GskWebGPUDamageTracker;

struct _GskWebGPUImage
{
  GskGpuImage parent_instance;

  /* WebGPU texture and view */
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUTextureFormat format;

  /* Resource state tracking */
  GskWebGPUResourceState current_state;
  GskWebGPUResourceState pending_state;

  /* Damage tracking for efficient updates */
  GskWebGPUDamageTracker *damage_tracker;

  /* Memory and performance tracking */
  gsize memory_size;
  gint64 creation_time;
  guint access_count;

  /* Synchronization */
  gboolean is_busy;
  GMutex state_mutex;
};

/* Image creation and management */
GskWebGPUImage *    gsk_webgpu_image_new                  (GskWebGPUDevice         *device,
                                                           WGPUTextureFormat        format,
                                                           gsize                    width,
                                                           gsize                    height);

GskWebGPUImage *    gsk_webgpu_image_new_for_texture      (GskWebGPUDevice         *device,
                                                           WGPUTexture              texture,
                                                           WGPUTextureFormat        format,
                                                           gsize                    width,
                                                           gsize                    height);

/* Resource state management */
void                gsk_webgpu_image_transition           (GskWebGPUImage          *self,
                                                           GskWebGPUResourceState   new_state);

GskWebGPUResourceState gsk_webgpu_image_get_state         (GskWebGPUImage          *self);

/* Damage tracking */
void                gsk_webgpu_image_mark_damage          (GskWebGPUImage          *self,
                                                           const cairo_rectangle_int_t *rect);

void                gsk_webgpu_image_mark_full_damage     (GskWebGPUImage          *self);

gboolean            gsk_webgpu_image_has_damage           (GskWebGPUImage          *self);

cairo_region_t *    gsk_webgpu_image_get_damage_region    (GskWebGPUImage          *self);

void                gsk_webgpu_image_clear_damage         (GskWebGPUImage          *self);

/* WebGPU texture access */
WGPUTexture         gsk_webgpu_image_get_texture          (GskWebGPUImage          *self);

WGPUTextureView     gsk_webgpu_image_get_view             (GskWebGPUImage          *self);

WGPUTextureFormat   gsk_webgpu_image_get_format           (GskWebGPUImage          *self);

/* Performance monitoring */
gsize               gsk_webgpu_image_get_memory_size      (GskWebGPUImage          *self);

guint               gsk_webgpu_image_get_access_count     (GskWebGPUImage          *self);

void                gsk_webgpu_image_record_access        (GskWebGPUImage          *self);

/* Utility functions */
const char *        gsk_webgpu_resource_state_to_string   (GskWebGPUResourceState   state);

GskWebGPUDamageTracker * gsk_webgpu_damage_tracker_new    (void);

void                gsk_webgpu_damage_tracker_free        (GskWebGPUDamageTracker  *tracker);

void                gsk_webgpu_damage_tracker_mark        (GskWebGPUDamageTracker  *tracker,
                                                           const cairo_rectangle_int_t *rect);

void                gsk_webgpu_damage_tracker_clear       (GskWebGPUDamageTracker  *tracker);

G_END_DECLS