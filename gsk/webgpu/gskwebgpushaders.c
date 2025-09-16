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

#include "gskwebgpushaders.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Load WGSL shader from embedded virtual filesystem */
char *gsk_webgpu_load_shader_file(const char *filename) {
  char filepath[512];
  
  // Try multiple locations in virtual filesystem
  const char *shader_paths[] = {
    "/shaders/%s",                    // Root shader directory
    "/gsk/webgpu/shaders/%s",        // GTK WebGPU shader directory  
    "/assets/shaders/%s",            // Assets directory
    "/usr/share/gtk-4.0/shaders/%s", // System shader directory
    NULL
  };
  
  for (int i = 0; shader_paths[i]; i++) {
    snprintf(filepath, sizeof(filepath), shader_paths[i], filename);
    
    FILE *file = fopen(filepath, "r");
    if (file) {
      // Successfully found shader file
      fseek(file, 0, SEEK_END);
      long length = ftell(file);
      fseek(file, 0, SEEK_SET);
      
      char *content = g_malloc(length + 1);
      size_t read_bytes = fread(content, 1, length, file);
      content[read_bytes] = '\0';
      
      fclose(file);
      
      g_debug("Loaded shader: %s (%ld bytes)", filepath, length);
      return content;
    }
  }
  
  g_warning("Failed to load shader file: %s (tried %d locations)", filename, i);
  return NULL;
}

/* Common vertex layout and functions used across all shaders */
const char *gsk_webgpu_get_vertex_layout_wgsl(void) {
  return 
    "struct VertexInput {\n"
    "  @location(0) position: vec2<f32>,\n"
    "  @location(1) tex_coord: vec2<f32>,\n"
    "  @location(2) color: vec4<f32>,\n"
    "}\n"
    "\n"
    "struct VertexOutput {\n"
    "  @builtin(position) position: vec4<f32>,\n"
    "  @location(0) tex_coord: vec2<f32>,\n"
    "  @location(1) color: vec4<f32>,\n"
    "  @location(2) world_pos: vec2<f32>,\n"
    "}\n";
}

const char *gsk_webgpu_get_common_functions_wgsl(void) {
  return
    "// Common utility functions for GTK rendering\n"
    "\n"
    "// SDF functions for rounded rectangles\n"
    "fn sdf_rounded_rect(p: vec2<f32>, size: vec2<f32>, radius: vec4<f32>) -> f32 {\n"
    "  let r = select(select(radius.xy, radius.zw, p.x > 0.0), select(radius.wz, radius.yx, p.x > 0.0), p.y > 0.0);\n"
    "  let q = abs(p) - size + r.x;\n"
    "  return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r.x;\n"
    "}\n"
    "\n"
    "// Premultiplied alpha blending\n"
    "fn premultiply_alpha(color: vec4<f32>) -> vec4<f32> {\n"
    "  return vec4<f32>(color.rgb * color.a, color.a);\n"
    "}\n"
    "\n"
    "// sRGB conversion functions\n"
    "fn linear_to_srgb(linear: vec3<f32>) -> vec3<f32> {\n"
    "  let cutoff = linear < vec3<f32>(0.0031308);\n"
    "  let higher = vec3<f32>(1.055) * pow(linear, vec3<f32>(1.0 / 2.4)) - vec3<f32>(0.055);\n"
    "  let lower = linear * 12.92;\n"
    "  return select(higher, lower, cutoff);\n"
    "}\n"
    "\n"
    "fn srgb_to_linear(srgb: vec3<f32>) -> vec3<f32> {\n"
    "  let cutoff = srgb < vec3<f32>(0.04045);\n"
    "  let higher = pow((srgb + vec3<f32>(0.055)) / vec3<f32>(1.055), vec3<f32>(2.4));\n"
    "  let lower = srgb / 12.92;\n"
    "  return select(higher, lower, cutoff);\n"
    "}\n"
    "\n"
    "// Gaussian blur weights\n"
    "fn gaussian_weight(x: f32, sigma: f32) -> f32 {\n"
    "  return exp(-(x * x) / (2.0 * sigma * sigma)) / (sigma * sqrt(2.0 * 3.14159265));\n"
    "}\n";
}

/* Basic 2D vertex shader for UI rendering - loaded from external file */
static char *gsk_webgpu_shader_vertex_2d = NULL;

const char *gsk_webgpu_get_shader_vertex_2d(void) {
  if (!gsk_webgpu_shader_vertex_2d) {
    char *common = gsk_webgpu_load_shader_file("common.wgsl");
    char *vertex = gsk_webgpu_load_shader_file("vertex_2d.wgsl");
    
    if (common && vertex) {
      // Combine common definitions with vertex shader
      size_t total_len = strlen(common) + strlen(vertex) + 1;
      gsk_webgpu_shader_vertex_2d = g_malloc(total_len);
      snprintf(gsk_webgpu_shader_vertex_2d, total_len, "%s\n%s", common, vertex);
      
      g_free(common);
      g_free(vertex);
    } else {
      g_warning("Failed to load vertex shader components");
      return NULL;
    }
  }
  
  return gsk_webgpu_shader_vertex_2d;
}

/* Solid color fragment shader - loaded from external file */
static char *gsk_webgpu_shader_color = NULL;

const char *gsk_webgpu_get_shader_color(void) {
  if (!gsk_webgpu_shader_color) {
    char *common = gsk_webgpu_load_shader_file("common.wgsl");
    char *color = gsk_webgpu_load_shader_file("color.wgsl");
    
    if (common && color) {
      size_t total_len = strlen(common) + strlen(color) + 1;
      gsk_webgpu_shader_color = g_malloc(total_len);
      snprintf(gsk_webgpu_shader_color, total_len, "%s\n%s", common, color);
      
      g_free(common);
      g_free(color);
    } else {
      g_warning("Failed to load color shader components");
      return NULL;
    }
  }
  
  return gsk_webgpu_shader_color;
}

/* Texture sampling fragment shader - loaded from external file */
static char *gsk_webgpu_shader_texture = NULL;

const char *gsk_webgpu_get_shader_texture(void) {
  if (!gsk_webgpu_shader_texture) {
    char *common = gsk_webgpu_load_shader_file("common.wgsl");
    char *texture = gsk_webgpu_load_shader_file("texture.wgsl");
    
    if (common && texture) {
      size_t total_len = strlen(common) + strlen(texture) + 1;
      gsk_webgpu_shader_texture = g_malloc(total_len);
      snprintf(gsk_webgpu_shader_texture, total_len, "%s\n%s", common, texture);
      
      g_free(common);
      g_free(texture);
    } else {
      g_warning("Failed to load texture shader components");
      return NULL;
    }
  }
  
  return gsk_webgpu_shader_texture;
}

/* Linear gradient fragment shader */
const char *gsk_webgpu_shader_linear_gradient = 
  "struct GradientUniforms {\n"
  "  start_point: vec2<f32>,\n"
  "  end_point: vec2<f32>,\n"
  "  color_stops: array<vec4<f32>, 8>,\n"
  "  stop_positions: array<f32, 8>,\n"
  "  stop_count: u32,\n"
  "  _padding: array<f32, 3>,\n"
  "}\n"
  "\n"
  "@group(1) @binding(0) var<uniform> gradient: GradientUniforms;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let gradient_vec = gradient.end_point - gradient.start_point;\n"
  "  let point_vec = input.world_pos - gradient.start_point;\n"
  "  \n"
  "  let t = clamp(dot(point_vec, gradient_vec) / dot(gradient_vec, gradient_vec), 0.0, 1.0);\n"
  "  \n"
  "  var color = gradient.color_stops[0];\n"
  "  \n"
  "  for (var i = 1u; i < gradient.stop_count; i = i + 1u) {\n"
  "    let prev_stop = gradient.stop_positions[i - 1u];\n"
  "    let curr_stop = gradient.stop_positions[i];\n"
  "    \n"
  "    if (t >= prev_stop && t <= curr_stop) {\n"
  "      let local_t = (t - prev_stop) / (curr_stop - prev_stop);\n"
  "      color = mix(gradient.color_stops[i - 1u], gradient.color_stops[i], local_t);\n"
  "      break;\n"
  "    }\n"
  "  }\n"
  "  \n"
  "  return premultiply_alpha(color * input.color);\n"
  "}\n";

/* Radial gradient fragment shader */
const char *gsk_webgpu_shader_radial_gradient = 
  "struct RadialGradientUniforms {\n"
  "  center: vec2<f32>,\n"
  "  radius: vec2<f32>,\n"
  "  color_stops: array<vec4<f32>, 8>,\n"
  "  stop_positions: array<f32, 8>,\n"
  "  stop_count: u32,\n"
  "  _padding: array<f32, 3>,\n"
  "}\n"
  "\n"
  "@group(1) @binding(0) var<uniform> gradient: RadialGradientUniforms;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let offset = input.world_pos - gradient.center;\n"
  "  let t = clamp(length(offset / gradient.radius), 0.0, 1.0);\n"
  "  \n"
  "  var color = gradient.color_stops[0];\n"
  "  \n"
  "  for (var i = 1u; i < gradient.stop_count; i = i + 1u) {\n"
  "    let prev_stop = gradient.stop_positions[i - 1u];\n"
  "    let curr_stop = gradient.stop_positions[i];\n"
  "    \n"
  "    if (t >= prev_stop && t <= curr_stop) {\n"
  "      let local_t = (t - prev_stop) / (curr_stop - prev_stop);\n"
  "      color = mix(gradient.color_stops[i - 1u], gradient.color_stops[i], local_t);\n"
  "      break;\n"
  "    }\n"
  "  }\n"
  "  \n"
  "  return premultiply_alpha(color * input.color);\n"
  "}\n";

/* Conic gradient fragment shader */
const char *gsk_webgpu_shader_conic_gradient = 
  "struct ConicGradientUniforms {\n"
  "  center: vec2<f32>,\n"
  "  angle: f32,\n"
  "  _padding: f32,\n"
  "  color_stops: array<vec4<f32>, 8>,\n"
  "  stop_positions: array<f32, 8>,\n"
  "  stop_count: u32,\n"
  "  _padding2: array<f32, 3>,\n"
  "}\n"
  "\n"
  "@group(1) @binding(0) var<uniform> gradient: ConicGradientUniforms;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let offset = input.world_pos - gradient.center;\n"
  "  let angle = atan2(offset.y, offset.x) + gradient.angle;\n"
  "  let t = fract((angle / (2.0 * 3.14159265)) + 0.5);\n"
  "  \n"
  "  var color = gradient.color_stops[0];\n"
  "  \n"
  "  for (var i = 1u; i < gradient.stop_count; i = i + 1u) {\n"
  "    let prev_stop = gradient.stop_positions[i - 1u];\n"
  "    let curr_stop = gradient.stop_positions[i];\n"
  "    \n"
  "    if (t >= prev_stop && t <= curr_stop) {\n"
  "      let local_t = (t - prev_stop) / (curr_stop - prev_stop);\n"
  "      color = mix(gradient.color_stops[i - 1u], gradient.color_stops[i], local_t);\n"
  "      break;\n"
  "    }\n"
  "  }\n"
  "  \n"
  "  return premultiply_alpha(color * input.color);\n"
  "}\n";

/* Border rendering shader */
const char *gsk_webgpu_shader_border = 
  "struct BorderUniforms {\n"
  "  rect: vec4<f32>,\n"
  "  border_widths: vec4<f32>,\n"
  "  border_colors: array<vec4<f32>, 4>,\n"
  "  corner_radii: vec4<f32>,\n"
  "}\n"
  "\n"
  "@group(1) @binding(0) var<uniform> border: BorderUniforms;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let rect_pos = input.world_pos - border.rect.xy;\n"
  "  let rect_size = border.rect.zw - border.rect.xy;\n"
  "  \n"
  "  // Determine which border edge we're in\n"
  "  let left_border = rect_pos.x < border.border_widths.x;\n"
  "  let right_border = rect_pos.x > rect_size.x - border.border_widths.z;\n"
  "  let top_border = rect_pos.y < border.border_widths.y;\n"
  "  let bottom_border = rect_pos.y > rect_size.y - border.border_widths.w;\n"
  "  \n"
  "  var border_color = vec4<f32>(0.0);\n"
  "  \n"
  "  if (left_border) {\n"
  "    border_color = border.border_colors[0]; // Left\n"
  "  } else if (right_border) {\n"
  "    border_color = border.border_colors[2]; // Right\n"
  "  }\n"
  "  \n"
  "  if (top_border) {\n"
  "    if (border_color.a > 0.0) {\n"
  "      border_color = mix(border_color, border.border_colors[1], 0.5); // Top\n"
  "    } else {\n"
  "      border_color = border.border_colors[1]; // Top\n"
  "    }\n"
  "  } else if (bottom_border) {\n"
  "    if (border_color.a > 0.0) {\n"
  "      border_color = mix(border_color, border.border_colors[3], 0.5); // Bottom\n"
  "    } else {\n"
  "      border_color = border.border_colors[3]; // Bottom\n"
  "    }\n"
  "  }\n"
  "  \n"
  "  if (border_color.a == 0.0) {\n"
  "    discard;\n"
  "  }\n"
  "  \n"
  "  return premultiply_alpha(border_color * input.color);\n"
  "}\n";

/* Rounded rectangle clipping shader */
const char *gsk_webgpu_shader_rounded_rect = 
  "struct RoundedRectUniforms {\n"
  "  rect: vec4<f32>,\n"
  "  corner_radii: vec4<f32>,\n"
  "}\n"
  "\n"
  "@group(1) @binding(0) var<uniform> rounded_rect: RoundedRectUniforms;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let rect_center = (rounded_rect.rect.xy + rounded_rect.rect.zw) * 0.5;\n"
  "  let rect_size = (rounded_rect.rect.zw - rounded_rect.rect.xy) * 0.5;\n"
  "  let p = input.world_pos - rect_center;\n"
  "  \n"
  "  let distance = sdf_rounded_rect(p, rect_size, rounded_rect.corner_radii);\n"
  "  \n"
  "  if (distance > 0.0) {\n"
  "    discard;\n"
  "  }\n"
  "  \n"
  "  let alpha = 1.0 - smoothstep(-0.5, 0.5, distance);\n"
  "  \n"
  "  return premultiply_alpha(input.color * alpha);\n"
  "}\n";

/* Color matrix transformation shader */
const char *gsk_webgpu_shader_color_matrix = 
  "struct ColorMatrixUniforms {\n"
  "  matrix: mat4x4<f32>,\n"
  "  offset: vec4<f32>,\n"
  "}\n"
  "\n"
  "@group(1) @binding(0) var texture_sampler: sampler;\n"
  "@group(1) @binding(1) var texture_2d: texture_2d<f32>;\n"
  "@group(1) @binding(2) var<uniform> color_matrix: ColorMatrixUniforms;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let tex_color = textureSample(texture_2d, texture_sampler, input.tex_coord);\n"
  "  let transformed = color_matrix.matrix * tex_color + color_matrix.offset;\n"
  "  return premultiply_alpha(clamp(transformed, vec4<f32>(0.0), vec4<f32>(1.0)) * input.color);\n"
  "}\n";

/* Gaussian blur compute shader (horizontal pass) */
const char *gsk_webgpu_shader_blur_compute_h = 
  "@group(0) @binding(0) var input_texture: texture_2d<f32>;\n"
  "@group(0) @binding(1) var output_texture: texture_storage_2d<rgba8unorm, write>;\n"
  "\n"
  "struct BlurUniforms {\n"
  "  blur_radius: f32,\n"
  "  sigma: f32,\n"
  "  texture_size: vec2<u32>,\n"
  "}\n"
  "\n"
  "@group(0) @binding(2) var<uniform> blur_params: BlurUniforms;\n"
  "\n"
  "@compute @workgroup_size(8, 8)\n"
  "fn cs_main(@builtin(global_invocation_id) global_id: vec3<u32>) {\n"
  "  let coords = vec2<i32>(global_id.xy);\n"
  "  \n"
  "  if (coords.x >= i32(blur_params.texture_size.x) || coords.y >= i32(blur_params.texture_size.y)) {\n"
  "    return;\n"
  "  }\n"
  "  \n"
  "  let radius = i32(blur_params.blur_radius);\n"
  "  var color = vec4<f32>(0.0);\n"
  "  var total_weight = 0.0;\n"
  "  \n"
  "  for (var x = -radius; x <= radius; x++) {\n"
  "    let sample_coord = coords + vec2<i32>(x, 0);\n"
  "    \n"
  "    if (sample_coord.x >= 0 && sample_coord.x < i32(blur_params.texture_size.x)) {\n"
  "      let weight = gaussian_weight(f32(x), blur_params.sigma);\n"
  "      color += textureLoad(input_texture, sample_coord, 0) * weight;\n"
  "      total_weight += weight;\n"
  "    }\n"
  "  }\n"
  "  \n"
  "  color /= total_weight;\n"
  "  textureStore(output_texture, coords, color);\n"
  "}\n";

/* Gaussian blur compute shader (vertical pass) */
const char *gsk_webgpu_shader_blur_compute_v = 
  "@group(0) @binding(0) var input_texture: texture_2d<f32>;\n"
  "@group(0) @binding(1) var output_texture: texture_storage_2d<rgba8unorm, write>;\n"
  "\n"
  "struct BlurUniforms {\n"
  "  blur_radius: f32,\n"
  "  sigma: f32,\n"
  "  texture_size: vec2<u32>,\n"
  "}\n"
  "\n"
  "@group(0) @binding(2) var<uniform> blur_params: BlurUniforms;\n"
  "\n"
  "@compute @workgroup_size(8, 8)\n"
  "fn cs_main(@builtin(global_invocation_id) global_id: vec3<u32>) {\n"
  "  let coords = vec2<i32>(global_id.xy);\n"
  "  \n"
  "  if (coords.x >= i32(blur_params.texture_size.x) || coords.y >= i32(blur_params.texture_size.y)) {\n"
  "    return;\n"
  "  }\n"
  "  \n"
  "  let radius = i32(blur_params.blur_radius);\n"
  "  var color = vec4<f32>(0.0);\n"
  "  var total_weight = 0.0;\n"
  "  \n"
  "  for (var y = -radius; y <= radius; y++) {\n"
  "    let sample_coord = coords + vec2<i32>(0, y);\n"
  "    \n"
  "    if (sample_coord.y >= 0 && sample_coord.y < i32(blur_params.texture_size.y)) {\n"
  "      let weight = gaussian_weight(f32(y), blur_params.sigma);\n"
  "      color += textureLoad(input_texture, sample_coord, 0) * weight;\n"
  "      total_weight += weight;\n"
  "    }\n"
  "  }\n"
  "  \n"
  "  color /= total_weight;\n"
  "  textureStore(output_texture, coords, color);\n"
  "}\n";

/* Text rendering vertex shader */
const char *gsk_webgpu_shader_text_vertex = 
  "struct TextUniforms {\n"
  "  mvp_matrix: mat4x4<f32>,\n"
  "  color: vec4<f32>,\n"
  "}\n"
  "\n"
  "@group(0) @binding(0) var<uniform> uniforms: TextUniforms;\n"
  "\n"
  "struct TextVertexInput {\n"
  "  @location(0) position: vec2<f32>,\n"
  "  @location(1) tex_coord: vec2<f32>,\n"
  "}\n"
  "\n"
  "struct TextVertexOutput {\n"
  "  @builtin(position) position: vec4<f32>,\n"
  "  @location(0) tex_coord: vec2<f32>,\n"
  "}\n"
  "\n"
  "@vertex fn vs_main(input: TextVertexInput) -> TextVertexOutput {\n"
  "  var output: TextVertexOutput;\n"
  "  output.position = uniforms.mvp_matrix * vec4<f32>(input.position, 0.0, 1.0);\n"
  "  output.tex_coord = input.tex_coord;\n"
  "  return output;\n"
  "}\n";

/* Text rendering fragment shader with SDF */
const char *gsk_webgpu_shader_text_fragment = 
  "struct TextUniforms {\n"
  "  mvp_matrix: mat4x4<f32>,\n"
  "  color: vec4<f32>,\n"
  "}\n"
  "\n"
  "@group(0) @binding(0) var<uniform> uniforms: TextUniforms;\n"
  "@group(1) @binding(0) var font_atlas: texture_2d<f32>;\n"
  "@group(1) @binding(1) var font_sampler: sampler;\n"
  "\n"
  "struct TextVertexOutput {\n"
  "  @builtin(position) position: vec4<f32>,\n"
  "  @location(0) tex_coord: vec2<f32>,\n"
  "}\n"
  "\n"
  "@fragment fn fs_main(input: TextVertexOutput) -> @location(0) vec4<f32> {\n"
  "  let distance = textureSample(font_atlas, font_sampler, input.tex_coord).r;\n"
  "  \n"
  "  // SDF text rendering with anti-aliasing\n"
  "  let smoothing = fwidth(distance);\n"
  "  let alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);\n"
  "  \n"
  "  if (alpha < 0.01) {\n"
  "    discard;\n"
  "  }\n"
  "  \n"
  "  return premultiply_alpha(vec4<f32>(uniforms.color.rgb, uniforms.color.a * alpha));\n"
  "}\n";

/* Blend modes - implementing all CSS blend modes */

const char *gsk_webgpu_shader_blend_normal = 
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  return input.color;\n"
  "}\n";

const char *gsk_webgpu_shader_blend_multiply = 
  "@group(1) @binding(0) var texture_sampler: sampler;\n"
  "@group(1) @binding(1) var source_texture: texture_2d<f32>;\n"
  "@group(1) @binding(2) var dest_texture: texture_2d<f32>;\n"
  "\n"
  "@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {\n"
  "  let source = textureSample(source_texture, texture_sampler, input.tex_coord);\n"
  "  let dest = textureSample(dest_texture, texture_sampler, input.tex_coord);\n"
  "  return premultiply_alpha(vec4<f32>(source.rgb * dest.rgb, source.a * dest.a));\n"
  "}\n";

/* Helper function to create shader modules */
WGPUShaderModule 
gsk_webgpu_create_shader_module (WGPUDevice   device,
                                  const char  *label,
                                  const char  *source)
{
  /* Combine common functions with the specific shader source */
  GString *full_source = g_string_new ("");
  g_string_append (full_source, gsk_webgpu_get_vertex_layout_wgsl ());
  g_string_append (full_source, "\n");
  g_string_append (full_source, gsk_webgpu_get_common_functions_wgsl ());
  g_string_append (full_source, "\n");
  g_string_append (full_source, source);

  WGPUShaderModuleWGSLDescriptor wgsl_desc = {
    .chain = {
      .sType = WGPUSType_ShaderModuleWGSLDescriptor
    },
    .source = full_source->str
  };

  WGPUShaderModuleDescriptor module_desc = {
    .label = label,
    .nextInChain = &wgsl_desc.chain
  };

  WGPUShaderModule module = wgpuDeviceCreateShaderModule (device, &module_desc);
  
  g_string_free (full_source, TRUE);
  
  return module;
}