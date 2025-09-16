/*
 * Masking operations shader - Complete WGSL implementation
 * Equivalent to gskgpumask.glsl from desktop GPU renderer
 * Copyright © 2025 Superstruct Ltd, New Zealand  
 * Licensed under LGPL-2.1-or-later
 */

@group(1) @binding(0) var mask_sampler: sampler;
@group(1) @binding(1) var source_texture: texture_2d<f32>;
@group(1) @binding(2) var mask_texture: texture_2d<f32>;

struct MaskUniforms {
  // Source and mask texture matrices
  source_matrix: mat4x4<f32>,
  mask_matrix: mat4x4<f32>,
  
  // Mask mode (0 = alpha, 1 = luminance)
  mask_mode: u32,
  
  // Color transformation
  color_matrix: mat4x4<f32>,
  
  _padding: array<f32, 3>,
}

@group(1) @binding(3) var<uniform> mask_uniforms: MaskUniforms;

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  // Transform texture coordinates for source
  let source_coord_4 = mask_uniforms.source_matrix * vec4<f32>(input.tex_coord, 0.0, 1.0);
  let source_coord = source_coord_4.xy / source_coord_4.w;
  
  // Transform texture coordinates for mask
  let mask_coord_4 = mask_uniforms.mask_matrix * vec4<f32>(input.tex_coord, 0.0, 1.0);
  let mask_coord = mask_coord_4.xy / mask_coord_4.w;
  
  // Sample source texture
  var source_color = textureSample(source_texture, mask_sampler, source_coord);
  
  // Sample mask texture
  let mask_sample = textureSample(mask_texture, mask_sampler, mask_coord);
  
  // Calculate mask value based on mode
  var mask_value: f32;
  if (mask_uniforms.mask_mode == 0u) {
    // Alpha masking
    mask_value = mask_sample.a;
  } else {
    // Luminance masking  
    mask_value = 0.299 * mask_sample.r + 0.587 * mask_sample.g + 0.114 * mask_sample.b;
  }
  
  // Apply color transformation
  source_color = mask_uniforms.color_matrix * source_color;
  
  // Apply mask
  return source_color * mask_value;
}