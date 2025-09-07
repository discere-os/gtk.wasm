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

#include <emscripten/html5_webgpu.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * Comprehensive WGSL shader library for GTK widget rendering.
 * These shaders implement the complete GSK rendering pipeline in WebGPU.
 */

/* Basic vertex shader for 2D rendering */
extern const char *gsk_webgpu_shader_vertex_2d;

/* Fragment shaders for different render node types */
extern const char *gsk_webgpu_shader_color;
extern const char *gsk_webgpu_shader_texture;
extern const char *gsk_webgpu_shader_linear_gradient;
extern const char *gsk_webgpu_shader_radial_gradient;
extern const char *gsk_webgpu_shader_conic_gradient;
extern const char *gsk_webgpu_shader_border;
extern const char *gsk_webgpu_shader_rounded_rect;
extern const char *gsk_webgpu_shader_inset_shadow;
extern const char *gsk_webgpu_shader_outset_shadow;
extern const char *gsk_webgpu_shader_blur;
extern const char *gsk_webgpu_shader_color_matrix;

/* Text rendering shaders */
extern const char *gsk_webgpu_shader_text_vertex;
extern const char *gsk_webgpu_shader_text_fragment;

/* Compute shaders for image processing */
extern const char *gsk_webgpu_shader_blur_compute_h;
extern const char *gsk_webgpu_shader_blur_compute_v;
extern const char *gsk_webgpu_shader_color_matrix_compute;
extern const char *gsk_webgpu_shader_image_scale_compute;

/* Blend mode shaders */
extern const char *gsk_webgpu_shader_blend_normal;
extern const char *gsk_webgpu_shader_blend_multiply;
extern const char *gsk_webgpu_shader_blend_screen;
extern const char *gsk_webgpu_shader_blend_overlay;
extern const char *gsk_webgpu_shader_blend_darken;
extern const char *gsk_webgpu_shader_blend_lighten;
extern const char *gsk_webgpu_shader_blend_color_dodge;
extern const char *gsk_webgpu_shader_blend_color_burn;
extern const char *gsk_webgpu_shader_blend_hard_light;
extern const char *gsk_webgpu_shader_blend_soft_light;
extern const char *gsk_webgpu_shader_blend_difference;
extern const char *gsk_webgpu_shader_blend_exclusion;

/* Helper functions */
WGPUShaderModule gsk_webgpu_create_shader_module      (WGPUDevice   device,
                                                        const char  *label,
                                                        const char  *source);

const char *     gsk_webgpu_get_vertex_layout_wgsl    (void);
const char *     gsk_webgpu_get_common_functions_wgsl (void);

G_END_DECLS