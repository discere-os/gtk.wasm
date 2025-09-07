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

#include "gskwebgpurenderer.h"
#include "gskrendernode.h"
#include <emscripten/html5_webgpu.h>

G_BEGIN_DECLS

/**
 * WebGPU render node visitor function for the GSK scene graph
 */
void gsk_webgpu_renderer_render_node (GskWebGPURenderer *renderer,
                                       GskRenderNode     *node);

/* Specific render node implementations */
void gsk_webgpu_renderer_render_container_node      (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_cairo_node          (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_color_node          (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_texture_node        (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_linear_gradient_node (GskWebGPURenderer *renderer,
                                                       GskRenderNode     *node);
void gsk_webgpu_renderer_render_radial_gradient_node (GskWebGPURenderer *renderer,
                                                       GskRenderNode     *node);
void gsk_webgpu_renderer_render_conic_gradient_node  (GskWebGPURenderer *renderer,
                                                       GskRenderNode     *node);
void gsk_webgpu_renderer_render_border_node         (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_inset_shadow_node   (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_outset_shadow_node  (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_transform_node      (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_opacity_node        (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_color_matrix_node   (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_repeat_node         (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_clip_node           (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_rounded_clip_node   (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_shadow_node         (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_blend_node          (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_cross_fade_node     (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_text_node           (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_blur_node           (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_debug_node          (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_gl_shader_node      (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);

/* GTK 4.10+ render nodes */
void gsk_webgpu_renderer_render_texture_scale_node  (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_mask_node           (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);

/* GTK 4.14+ render nodes */
void gsk_webgpu_renderer_render_stroke_node         (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_fill_node           (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);
void gsk_webgpu_renderer_render_subsurface_node     (GskWebGPURenderer *renderer,
                                                      GskRenderNode     *node);

G_END_DECLS