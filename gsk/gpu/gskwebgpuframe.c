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

#include "gskwebgpuframeprivate.h"
#include "gskwebgpudeviceprivate.h"
#include "gskwebgpuimageprivate.h"

#include <webgpu/webgpu.h>

struct _GskWebGPUFrame
{
  GskGpuFrame parent_instance;

  GskWebGPUDevice *device;
  WGPUCommandEncoder command_encoder;
  WGPURenderPassEncoder render_pass;

  /* Resource state tracking following D3D12 patterns */
  GHashTable *resource_states;

  /* Current render state */
  WGPURenderPipeline current_pipeline;
  WGPUBindGroup current_bind_groups[8]; /* WebGPU max bind groups */
  guint32 bound_bind_group_count;

  /* Performance tracking */
  guint32 draw_call_count;
  guint32 triangle_count;
  guint32 texture_binding_count;

  /* Damage tracking */
  GskWebGPUDamageTracker *damage_tracker;
  GskWebGPUDamageRect *accumulated_damage;

  /* Command submission state */
  gboolean is_recording;
  gboolean has_render_pass;
};

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

G_DEFINE_FINAL_TYPE (GskWebGPUFrame, gsk_webgpu_frame, GSK_TYPE_GPU_FRAME)

static void
gsk_webgpu_frame_setup (GskGpuFrame *frame,
                        GskGpuImage *image,
                        GskGpuImage *depth_stencil)
{
  GskWebGPUFrame *self = GSK_WEBGPU_FRAME (frame);
  GskWebGPUImage *webgpu_image = GSK_WEBGPU_IMAGE (image);
  WGPUTextureView color_view, depth_view = 0;
  WGPURenderPassDescriptor render_pass_desc;
  WGPURenderPassColorAttachment color_attachment;
  WGPURenderPassDepthStencilAttachment depth_attachment;

  g_return_if_fail (!self->has_render_pass);

  /* Transition image to render target state */
  gsk_webgpu_frame_transition_resource (self,
                                        gsk_webgpu_image_get_texture (webgpu_image),
                                        GSK_WEBGPU_RESOURCE_STATE_RENDER_TARGET);

  /* Get texture views */
  color_view = gsk_webgpu_image_get_view (webgpu_image);
  if (depth_stencil)
    depth_view = gsk_webgpu_image_get_view (GSK_WEBGPU_IMAGE (depth_stencil));

  /* Set up color attachment */
  color_attachment = (WGPURenderPassColorAttachment) {
    .view = color_view,
    .resolveTarget = 0,
    .loadOp = WGPULoadOp_Clear,
    .storeOp = WGPUStoreOp_Store,
    .clearValue = { 0.0, 0.0, 0.0, 1.0 }
  };

  /* Set up render pass descriptor */
  render_pass_desc = (WGPURenderPassDescriptor) {
    .label = "GskWebGPUFrame render pass",
    .colorAttachmentCount = 1,
    .colorAttachments = &color_attachment,
    .depthStencilAttachment = NULL
  };

  /* Add depth attachment if provided */
  if (depth_view)
    {
      depth_attachment = (WGPURenderPassDepthStencilAttachment) {
        .view = depth_view,
        .depthLoadOp = WGPULoadOp_Clear,
        .depthStoreOp = WGPUStoreOp_Store,
        .depthClearValue = 1.0,
        .stencilLoadOp = WGPULoadOp_Clear,
        .stencilStoreOp = WGPUStoreOp_Store,
        .stencilClearValue = 0
      };
      render_pass_desc.depthStencilAttachment = &depth_attachment;
    }

  /* Begin render pass */
  self->render_pass = wgpuCommandEncoderBeginRenderPass (self->command_encoder,
                                                          &render_pass_desc);
  self->has_render_pass = TRUE;

  /* Update damage tracking */
  if (self->damage_tracker)
    {
      GskWebGPUDamageRect damage = {
        .x = 0, .y = 0,
        .width = gsk_webgpu_image_get_width (webgpu_image),
        .height = gsk_webgpu_image_get_height (webgpu_image)
      };

      gsk_webgpu_damage_tracker_add (self->damage_tracker,
                                     gsk_webgpu_image_get_texture (webgpu_image),
                                     &damage);
    }
}

static void
gsk_webgpu_frame_cleanup (GskGpuFrame *frame)
{
  GskWebGPUFrame *self = GSK_WEBGPU_FRAME (frame);

  if (self->has_render_pass)
    {
      wgpuRenderPassEncoderEnd (self->render_pass);
      wgpuRenderPassEncoderRelease (self->render_pass);
      self->render_pass = 0;
      self->has_render_pass = FALSE;
    }

  /* Update device performance counters */
  gsk_webgpu_device_update_performance_counters (self->device,
                                                 self->draw_call_count,
                                                 self->triangle_count);

  /* Reset frame counters */
  self->draw_call_count = 0;
  self->triangle_count = 0;
  self->texture_binding_count = 0;
}

static void
gsk_webgpu_frame_submit (GskGpuFrame *frame)
{
  GskWebGPUFrame *self = GSK_WEBGPU_FRAME (frame);
  WGPUCommandBuffer command_buffer;
  WGPUQueue queue;

  if (!self->is_recording)
    return;

  /* End render pass if still active */
  if (self->has_render_pass)
    {
      wgpuRenderPassEncoderEnd (self->render_pass);
      wgpuRenderPassEncoderRelease (self->render_pass);
      self->render_pass = 0;
      self->has_render_pass = FALSE;
    }

  /* Finish command encoding */
  command_buffer = wgpuCommandEncoderFinish (self->command_encoder,
                                            &(WGPUCommandBufferDescriptor) {
                                              .label = "GskWebGPUFrame commands"
                                            });

  /* Submit to queue */
  queue = gsk_webgpu_device_get_queue (self->device);
  wgpuQueueSubmit (queue, 1, &command_buffer);

  /* Cleanup */
  wgpuCommandBufferRelease (command_buffer);
  wgpuCommandEncoderRelease (self->command_encoder);
  self->command_encoder = 0;
  self->is_recording = FALSE;
}

static void
gsk_webgpu_frame_finalize (GObject *object)
{
  GskWebGPUFrame *self = GSK_WEBGPU_FRAME (object);

  g_clear_pointer (&self->resource_states, g_hash_table_unref);
  g_clear_pointer (&self->accumulated_damage, g_free);

  if (self->has_render_pass)
    {
      wgpuRenderPassEncoderEnd (self->render_pass);
      wgpuRenderPassEncoderRelease (self->render_pass);
    }

  if (self->command_encoder)
    {
      wgpuCommandEncoderRelease (self->command_encoder);
    }

  g_clear_object (&self->device);

  G_OBJECT_CLASS (gsk_webgpu_frame_parent_class)->finalize (object);
}

static void
gsk_webgpu_frame_class_init (GskWebGPUFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GskGpuFrameClass *frame_class = GSK_GPU_FRAME_CLASS (klass);

  object_class->finalize = gsk_webgpu_frame_finalize;

  frame_class->setup = gsk_webgpu_frame_setup;
  frame_class->cleanup = gsk_webgpu_frame_cleanup;
  frame_class->submit = gsk_webgpu_frame_submit;
}

static void
gsk_webgpu_frame_init (GskWebGPUFrame *self)
{
  self->device = NULL;
  self->command_encoder = 0;
  self->render_pass = 0;

  self->resource_states = g_hash_table_new (g_direct_hash, g_direct_equal);

  self->current_pipeline = 0;
  memset (self->current_bind_groups, 0, sizeof (self->current_bind_groups));
  self->bound_bind_group_count = 0;

  self->draw_call_count = 0;
  self->triangle_count = 0;
  self->texture_binding_count = 0;

  self->damage_tracker = NULL;
  self->accumulated_damage = NULL;

  self->is_recording = FALSE;
  self->has_render_pass = FALSE;
}

GskGpuFrame *
gsk_webgpu_frame_new (GskWebGPUDevice *device,
                      GskGpuImage     *image)
{
  GskWebGPUFrame *self;
  WGPUDevice wgpu_device;

  g_return_val_if_fail (GSK_IS_WEBGPU_DEVICE (device), NULL);

  wgpu_device = gsk_webgpu_device_get_device (device);

  self = g_object_new (GSK_TYPE_WEBGPU_FRAME, NULL);
  self->device = g_object_ref (device);
  self->damage_tracker = gsk_webgpu_device_get_damage_tracker (device);

  /* Create command encoder */
  self->command_encoder = wgpuDeviceCreateCommandEncoder (wgpu_device,
                                                          &(WGPUCommandEncoderDescriptor) {
                                                            .label = "GskWebGPUFrame encoder"
                                                          });
  self->is_recording = TRUE;

  return GSK_GPU_FRAME (self);
}

/* Resource state management following D3D12 patterns */
void
gsk_webgpu_frame_transition_resource (GskWebGPUFrame        *self,
                                      WGPUTexture            texture,
                                      GskWebGPUResourceState new_state)
{
  GskWebGPUResourceState *current_state;

  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (texture != 0);

  current_state = g_hash_table_lookup (self->resource_states,
                                       GSIZE_TO_POINTER ((gsize)texture));

  if (!current_state)
    {
      current_state = g_new (GskWebGPUResourceState, 1);
      *current_state = GSK_WEBGPU_RESOURCE_STATE_UNDEFINED;
      g_hash_table_insert (self->resource_states,
                           GSIZE_TO_POINTER ((gsize)texture),
                           current_state);
    }

  if (*current_state == new_state)
    return;

  /* WebGPU handles resource transitions implicitly,
   * but we track them for optimization and debugging */
  *current_state = new_state;

  GSK_DEBUG (RENDERER, "Resource transition: texture=%p, state=%d", texture, new_state);
}

/* Command recording */
void
gsk_webgpu_frame_set_pipeline (GskWebGPUFrame     *self,
                               WGPURenderPipeline  pipeline)
{
  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (self->has_render_pass);
  g_return_if_fail (pipeline != 0);

  if (self->current_pipeline == pipeline)
    return;

  wgpuRenderPassEncoderSetPipeline (self->render_pass, pipeline);
  self->current_pipeline = pipeline;

  /* Clear bind groups when pipeline changes */
  memset (self->current_bind_groups, 0, sizeof (self->current_bind_groups));
  self->bound_bind_group_count = 0;
}

void
gsk_webgpu_frame_set_bind_group (GskWebGPUFrame *self,
                                 guint32         index,
                                 WGPUBindGroup   bind_group)
{
  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (self->has_render_pass);
  g_return_if_fail (index < G_N_ELEMENTS (self->current_bind_groups));
  g_return_if_fail (bind_group != 0);

  if (self->current_bind_groups[index] == bind_group)
    return;

  wgpuRenderPassEncoderSetBindGroup (self->render_pass, index, bind_group, 0, NULL);
  self->current_bind_groups[index] = bind_group;
  self->texture_binding_count++;

  if (index >= self->bound_bind_group_count)
    self->bound_bind_group_count = index + 1;
}

void
gsk_webgpu_frame_set_vertex_buffer (GskWebGPUFrame *self,
                                    guint32         slot,
                                    WGPUBuffer      buffer,
                                    guint64         offset,
                                    guint64         size)
{
  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (self->has_render_pass);
  g_return_if_fail (buffer != 0);

  /* Transition buffer to vertex buffer state */
  gsk_webgpu_frame_transition_resource (self, (WGPUTexture)buffer,
                                        GSK_WEBGPU_RESOURCE_STATE_VERTEX_BUFFER);

  wgpuRenderPassEncoderSetVertexBuffer (self->render_pass, slot, buffer, offset, size);
}

void
gsk_webgpu_frame_set_index_buffer (GskWebGPUFrame  *self,
                                   WGPUBuffer       buffer,
                                   WGPUIndexFormat  format,
                                   guint64          offset,
                                   guint64          size)
{
  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (self->has_render_pass);
  g_return_if_fail (buffer != 0);

  /* Transition buffer to index buffer state */
  gsk_webgpu_frame_transition_resource (self, (WGPUTexture)buffer,
                                        GSK_WEBGPU_RESOURCE_STATE_INDEX_BUFFER);

  wgpuRenderPassEncoderSetIndexBuffer (self->render_pass, buffer, format, offset, size);
}

void
gsk_webgpu_frame_draw (GskWebGPUFrame *self,
                       guint32         vertex_count,
                       guint32         instance_count,
                       guint32         first_vertex,
                       guint32         first_instance)
{
  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (self->has_render_pass);

  wgpuRenderPassEncoderDraw (self->render_pass,
                            vertex_count,
                            instance_count,
                            first_vertex,
                            first_instance);

  /* Update performance counters */
  self->draw_call_count++;
  self->triangle_count += (vertex_count / 3) * instance_count;
}

void
gsk_webgpu_frame_draw_indexed (GskWebGPUFrame *self,
                               guint32         index_count,
                               guint32         instance_count,
                               guint32         first_index,
                               gint32          base_vertex,
                               guint32         first_instance)
{
  g_return_if_fail (GSK_IS_WEBGPU_FRAME (self));
  g_return_if_fail (self->has_render_pass);

  wgpuRenderPassEncoderDrawIndexed (self->render_pass,
                                    index_count,
                                    instance_count,
                                    first_index,
                                    base_vertex,
                                    first_instance);

  /* Update performance counters */
  self->draw_call_count++;
  self->triangle_count += (index_count / 3) * instance_count;
}

/* Performance monitoring */
GskWebGPUPerformanceMetrics
gsk_webgpu_frame_get_performance_metrics (GskWebGPUFrame *self)
{
  GskWebGPUPerformanceMetrics metrics = { 0 };

  g_return_val_if_fail (GSK_IS_WEBGPU_FRAME (self), metrics);

  metrics.draw_call_count = self->draw_call_count;
  metrics.triangle_count = self->triangle_count;
  metrics = gsk_webgpu_device_get_performance_metrics (self->device);

  return metrics;
}