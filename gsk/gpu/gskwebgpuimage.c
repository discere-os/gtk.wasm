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

#include "gskwebgpuimageprivate.h"
#include "gskwebgpudeviceprivate.h"

#include <webgpu/webgpu.h>

G_DEFINE_TYPE(GskWebGPUImage, gsk_webgpu_image, GSK_TYPE_GPU_IMAGE)

static void
gsk_webgpu_image_finalize(GObject *object)
{
  GskWebGPUImage *self = GSK_WEBGPU_IMAGE(object);

  if (self->view)
    wgpuTextureViewRelease(self->view);

  if (self->texture)
    wgpuTextureRelease(self->texture);

  if (self->damage_tracker)
    gsk_webgpu_damage_tracker_free(self->damage_tracker);

  g_mutex_clear(&self->state_mutex);

  G_OBJECT_CLASS(gsk_webgpu_image_parent_class)->finalize(object);
}

static void
gsk_webgpu_image_class_init(GskWebGPUImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = gsk_webgpu_image_finalize;
}

static void
gsk_webgpu_image_init(GskWebGPUImage *self)
{
  self->texture = NULL;
  self->view = NULL;
  self->format = WGPUTextureFormat_Undefined;
  self->current_state = GSK_WEBGPU_RESOURCE_STATE_UNDEFINED;
  self->pending_state = GSK_WEBGPU_RESOURCE_STATE_UNDEFINED;
  self->damage_tracker = gsk_webgpu_damage_tracker_new();
  self->memory_size = 0;
  self->creation_time = g_get_monotonic_time();
  self->access_count = 0;
  self->is_busy = FALSE;

  g_mutex_init(&self->state_mutex);
}

GskWebGPUImage *
gsk_webgpu_image_new(GskWebGPUDevice *device,
                     WGPUTextureFormat format,
                     gsize width,
                     gsize height)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_DEVICE(device), NULL);

  WGPUDevice wgpu_device = gsk_webgpu_device_get_device(device);

  WGPUTextureDescriptor texture_desc = {
    .nextInChain = NULL,
    .label = "GSK WebGPU Image",
    .usage = WGPUTextureUsage_RenderAttachment |
             WGPUTextureUsage_TextureBinding |
             WGPUTextureUsage_CopyDst |
             WGPUTextureUsage_CopySrc,
    .dimension = WGPUTextureDimension_2D,
    .size = {
      .width = width,
      .height = height,
      .depthOrArrayLayers = 1
    },
    .format = format,
    .mipLevelCount = 1,
    .sampleCount = 1,
    .viewFormatCount = 0,
    .viewFormats = NULL
  };

  WGPUTexture texture = wgpuDeviceCreateTexture(wgpu_device, &texture_desc);
  if (!texture)
    return NULL;

  return gsk_webgpu_image_new_for_texture(device, texture, format, width, height);
}

GskWebGPUImage *
gsk_webgpu_image_new_for_texture(GskWebGPUDevice *device,
                                 WGPUTexture texture,
                                 WGPUTextureFormat format,
                                 gsize width,
                                 gsize height)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_DEVICE(device), NULL);
  g_return_val_if_fail(texture != NULL, NULL);

  GskWebGPUImage *self = g_object_new(GSK_TYPE_WEBGPU_IMAGE,
                                      "device", device,
                                      "width", width,
                                      "height", height,
                                      NULL);

  self->texture = texture;
  self->format = format;

  WGPUTextureViewDescriptor view_desc = {
    .nextInChain = NULL,
    .label = "GSK WebGPU Image View",
    .format = format,
    .dimension = WGPUTextureViewDimension_2D,
    .baseMipLevel = 0,
    .mipLevelCount = 1,
    .baseArrayLayer = 0,
    .arrayLayerCount = 1,
    .aspect = WGPUTextureAspect_All
  };

  self->view = wgpuTextureCreateView(texture, &view_desc);

  /* Calculate memory usage estimate */
  guint bytes_per_pixel = 4; /* Assume 32-bit RGBA */
  self->memory_size = width * height * bytes_per_pixel;

  return self;
}

void
gsk_webgpu_image_transition(GskWebGPUImage *self,
                           GskWebGPUResourceState new_state)
{
  g_return_if_fail(GSK_IS_WEBGPU_IMAGE(self));

  g_mutex_lock(&self->state_mutex);

  if (self->current_state != new_state)
    {
      self->pending_state = new_state;
      /* Resource barriers would be recorded here in a real implementation */
      self->current_state = new_state;
    }

  g_mutex_unlock(&self->state_mutex);
}

GskWebGPUResourceState
gsk_webgpu_image_get_state(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), GSK_WEBGPU_RESOURCE_STATE_UNDEFINED);

  return self->current_state;
}

void
gsk_webgpu_image_mark_damage(GskWebGPUImage *self,
                            const cairo_rectangle_int_t *rect)
{
  g_return_if_fail(GSK_IS_WEBGPU_IMAGE(self));

  if (self->damage_tracker && rect)
    gsk_webgpu_damage_tracker_mark(self->damage_tracker, rect);
}

void
gsk_webgpu_image_mark_full_damage(GskWebGPUImage *self)
{
  g_return_if_fail(GSK_IS_WEBGPU_IMAGE(self));

  if (self->damage_tracker)
    {
      self->damage_tracker->needs_full_update = TRUE;
      self->damage_tracker->last_update_time = g_get_monotonic_time();
    }
}

gboolean
gsk_webgpu_image_has_damage(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), FALSE);

  if (!self->damage_tracker)
    return FALSE;

  return self->damage_tracker->needs_full_update ||
         (self->damage_tracker->region && !cairo_region_is_empty(self->damage_tracker->region));
}

cairo_region_t *
gsk_webgpu_image_get_damage_region(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), NULL);

  if (!self->damage_tracker)
    return NULL;

  return self->damage_tracker->region;
}

void
gsk_webgpu_image_clear_damage(GskWebGPUImage *self)
{
  g_return_if_fail(GSK_IS_WEBGPU_IMAGE(self));

  if (self->damage_tracker)
    gsk_webgpu_damage_tracker_clear(self->damage_tracker);
}

WGPUTexture
gsk_webgpu_image_get_texture(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), NULL);

  return self->texture;
}

WGPUTextureView
gsk_webgpu_image_get_view(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), NULL);

  return self->view;
}

WGPUTextureFormat
gsk_webgpu_image_get_format(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), WGPUTextureFormat_Undefined);

  return self->format;
}

gsize
gsk_webgpu_image_get_memory_size(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), 0);

  return self->memory_size;
}

guint
gsk_webgpu_image_get_access_count(GskWebGPUImage *self)
{
  g_return_val_if_fail(GSK_IS_WEBGPU_IMAGE(self), 0);

  return self->access_count;
}

void
gsk_webgpu_image_record_access(GskWebGPUImage *self)
{
  g_return_if_fail(GSK_IS_WEBGPU_IMAGE(self));

  self->access_count++;
}

const char *
gsk_webgpu_resource_state_to_string(GskWebGPUResourceState state)
{
  switch (state)
    {
    case GSK_WEBGPU_RESOURCE_STATE_UNDEFINED:
      return "UNDEFINED";
    case GSK_WEBGPU_RESOURCE_STATE_RENDER_TARGET:
      return "RENDER_TARGET";
    case GSK_WEBGPU_RESOURCE_STATE_SHADER_RESOURCE:
      return "SHADER_RESOURCE";
    case GSK_WEBGPU_RESOURCE_STATE_COPY_SOURCE:
      return "COPY_SOURCE";
    case GSK_WEBGPU_RESOURCE_STATE_COPY_DEST:
      return "COPY_DEST";
    case GSK_WEBGPU_RESOURCE_STATE_PRESENT:
      return "PRESENT";
    default:
      return "UNKNOWN";
    }
}

/* Damage tracker implementation */
GskWebGPUDamageTracker *
gsk_webgpu_damage_tracker_new(void)
{
  GskWebGPUDamageTracker *tracker = g_new0(GskWebGPUDamageTracker, 1);
  tracker->region = cairo_region_create();
  tracker->needs_full_update = FALSE;
  tracker->last_update_time = 0;
  return tracker;
}

void
gsk_webgpu_damage_tracker_free(GskWebGPUDamageTracker *tracker)
{
  if (!tracker)
    return;

  if (tracker->region)
    cairo_region_destroy(tracker->region);

  g_free(tracker);
}

void
gsk_webgpu_damage_tracker_mark(GskWebGPUDamageTracker *tracker,
                              const cairo_rectangle_int_t *rect)
{
  if (!tracker || !rect)
    return;

  if (tracker->region)
    cairo_region_union_rectangle(tracker->region, rect);

  tracker->last_update_time = g_get_monotonic_time();
}

void
gsk_webgpu_damage_tracker_clear(GskWebGPUDamageTracker *tracker)
{
  if (!tracker)
    return;

  if (tracker->region)
    cairo_region_subtract(tracker->region, tracker->region);

  tracker->needs_full_update = FALSE;
  tracker->last_update_time = 0;
}