/*
 * Texture rendering fragment shader
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

@group(1) @binding(0) var texture_sampler: sampler;
@group(1) @binding(1) var texture_2d: texture_2d<f32>;

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  let tex_color = textureSample(texture_2d, texture_sampler, input.tex_coord);
  return premultiply_alpha(tex_color * input.color);
}