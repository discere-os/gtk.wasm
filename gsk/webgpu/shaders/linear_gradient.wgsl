/*
 * Linear gradient rendering fragment shader
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

struct GradientUniforms {
  start_point: vec2<f32>,
  end_point: vec2<f32>,
  color_stops: array<vec4<f32>, 8>,
  stop_positions: array<f32, 8>,
  stop_count: u32,
  _padding: array<f32, 3>,
}

@group(1) @binding(0) var<uniform> gradient: GradientUniforms;

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  let gradient_vec = gradient.end_point - gradient.start_point;
  let point_vec = input.world_pos - gradient.start_point;
  
  let t = clamp(dot(point_vec, gradient_vec) / dot(gradient_vec, gradient_vec), 0.0, 1.0);
  
  var color = gradient.color_stops[0];
  
  for (var i = 1u; i < gradient.stop_count; i = i + 1u) {
    let prev_stop = gradient.stop_positions[i - 1u];
    let curr_stop = gradient.stop_positions[i];
    
    if (t >= prev_stop && t <= curr_stop) {
      let local_t = (t - prev_stop) / (curr_stop - prev_stop);
      color = mix(gradient.color_stops[i - 1u], gradient.color_stops[i], local_t);
      break;
    }
  }
  
  return premultiply_alpha(color * input.color);
}