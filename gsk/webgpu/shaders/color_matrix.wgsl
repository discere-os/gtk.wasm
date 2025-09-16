/*
 * Color matrix transformation shader - Complete WGSL implementation
 * Equivalent to gskgpucolormatrix.glsl from desktop GPU renderer
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

@group(1) @binding(0) var color_sampler: sampler;
@group(1) @binding(1) var source_texture: texture_2d<f32>;

struct ColorMatrixUniforms {
  // 4x4 color transformation matrix
  color_matrix: mat4x4<f32>,
  
  // Color offset vector
  color_offset: vec4<f32>,
  
  // Texture coordinate transformation
  texture_matrix: mat4x4<f32>,
  
  // Alternative color space handling
  alt_color_matrix: mat4x4<f32>,
  
  _padding: array<f32, 4>,
}

@group(1) @binding(2) var<uniform> color_uniforms: ColorMatrixUniforms;

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  // Transform texture coordinates
  let tex_coord_4 = color_uniforms.texture_matrix * vec4<f32>(input.tex_coord, 0.0, 1.0);
  let tex_coord = tex_coord_4.xy / tex_coord_4.w;
  
  // Sample source texture
  var source_color = textureSample(source_texture, color_sampler, tex_coord);
  
  // Unpremultiply alpha for color matrix operations
  if (source_color.a > 0.001) {
    source_color = vec4<f32>(source_color.rgb / source_color.a, source_color.a);
  }
  
  // Apply color matrix transformation
  var transformed_color = color_uniforms.color_matrix * source_color;
  
  // Add color offset
  transformed_color += color_uniforms.color_offset;
  
  // Clamp to valid range
  transformed_color = clamp(transformed_color, vec4<f32>(0.0), vec4<f32>(1.0));
  
  // Premultiply alpha again
  return premultiply_alpha(transformed_color);
}