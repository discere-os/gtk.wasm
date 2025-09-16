/*
 * Gaussian blur fragment shader for GTK WebGPU rendering
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

@group(1) @binding(0) var blur_sampler: sampler;
@group(1) @binding(1) var source_texture: texture_2d<f32>;

struct BlurUniforms {
  blur_radius: f32,
  blur_direction: vec2<f32>,  // (1,0) for horizontal, (0,1) for vertical
  texture_size: vec2<f32>,
  _padding: f32,
}

@group(1) @binding(2) var<uniform> blur_uniforms: BlurUniforms;

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  let pixel_size = 1.0 / blur_uniforms.texture_size;
  let sigma = blur_uniforms.blur_radius / 3.0;
  
  var color = vec4<f32>(0.0);
  var total_weight = 0.0;
  
  // Sample in blur direction with Gaussian weights
  let sample_count = i32(blur_uniforms.blur_radius * 2.0) + 1;
  let half_samples = sample_count / 2;
  
  for (var i = -half_samples; i <= half_samples; i++) {
    let offset = f32(i) * pixel_size * blur_uniforms.blur_direction;
    let sample_coord = input.tex_coord + offset;
    
    let weight = gaussian_weight(f32(i), sigma);
    let sample_color = textureSample(source_texture, blur_sampler, sample_coord);
    
    color += sample_color * weight;
    total_weight += weight;
  }
  
  return color / total_weight;
}