/*
 * Radial gradient shader - Complete WGSL implementation
 * Equivalent to gskgpuradialgradient.glsl from desktop GPU renderer
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

struct RadialGradientUniforms {
  // Rectangle bounds  
  rect: vec4<f32>,
  
  // 7 color stops (matching desktop implementation)
  colors: array<vec4<f32>, 7>,
  
  // Color stop positions
  offsets: array<f32, 7>,
  
  // Color interpolation hints
  hints: array<f32, 7>,
  
  // Center position and radius (x, y, inner_radius, outer_radius)
  center_radius: vec4<f32>,
  
  // Start/end positions for gradient
  start_end: vec2<f32>,
  
  // Variation flags
  supersampling: u32,  // 0 or 1
  repeating: u32,      // 0 or 1
  
  // Scale factor
  global_scale: f32,
  
  _padding: f32,
}

@group(1) @binding(0) var<uniform> gradient: RadialGradientUniforms;

// Color interpolation with hints (matches desktop compute_c function)
fn compute_c(f: f32, hint: f32) -> f32 {
  if (hint == 0.5) {
    return f;
  } else if (hint <= 0.0) {
    return 1.0;
  } else if (hint >= 1.0) {
    return 0.0;
  } else {
    let ln2 = 0.69314718055994530942;
    return pow(f, -ln2 / log(hint));
  }
}

// Get gradient color at specific offset (matches desktop implementation)
fn get_gradient_color(offset: f32) -> vec4<f32> {
  var color: vec4<f32>;
  var f: f32;
  
  // 7-way branching logic matching desktop GLSL implementation
  if (offset <= gradient.offsets[3]) {
    if (offset <= gradient.offsets[1]) {
      if (offset <= gradient.offsets[0]) {
        color = gradient.colors[0];
      } else {
        f = (offset - gradient.offsets[0]) / (gradient.offsets[1] - gradient.offsets[0]);
        f = compute_c(f, gradient.hints[1]);
        color = mix(gradient.colors[0], gradient.colors[1], f);
      }
    } else {
      if (offset <= gradient.offsets[2]) {
        f = (offset - gradient.offsets[1]) / (gradient.offsets[2] - gradient.offsets[1]);
        f = compute_c(f, gradient.hints[2]);
        color = mix(gradient.colors[1], gradient.colors[2], f);
      } else {
        f = (offset - gradient.offsets[2]) / (gradient.offsets[3] - gradient.offsets[2]);
        f = compute_c(f, gradient.hints[3]);
        color = mix(gradient.colors[2], gradient.colors[3], f);
      }
    }
  } else {
    if (offset <= gradient.offsets[5]) {
      if (offset <= gradient.offsets[4]) {
        f = (offset - gradient.offsets[3]) / (gradient.offsets[4] - gradient.offsets[3]);
        f = compute_c(f, gradient.hints[4]);
        color = mix(gradient.colors[3], gradient.colors[4], f);
      } else {
        f = (offset - gradient.offsets[4]) / (gradient.offsets[5] - gradient.offsets[4]);
        f = compute_c(f, gradient.hints[5]);
        color = mix(gradient.colors[4], gradient.colors[5], f);
      }
    } else {
      if (offset <= gradient.offsets[6]) {
        f = (offset - gradient.offsets[5]) / (gradient.offsets[6] - gradient.offsets[5]);
        f = compute_c(f, gradient.hints[6]);
        color = mix(gradient.colors[5], gradient.colors[6], f);
      } else {
        color = gradient.colors[6];
      }
    }
  }
  
  return color;
}

// Get gradient color at world position
fn get_gradient_color_at(pos: vec2<f32>) -> vec4<f32> {
  var offset = length(pos / gradient.center_radius.zw);
  offset = (offset - gradient.start_end.x) / (gradient.start_end.y - gradient.start_end.x);
  
  if (gradient.repeating != 0u) {
    offset = fract(offset);
  } else {
    offset = clamp(offset, 0.0, 1.0);
  }
  
  return get_gradient_color(offset);
}

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  // Calculate rectangle coverage (matches desktop rect_coverage)
  let rect_min = gradient.rect.xy;
  let rect_max = gradient.rect.xy + gradient.rect.zw;
  let pos = input.world_pos;
  
  let alpha = select(0.0, 1.0, 
    all(pos >= rect_min) && all(pos <= rect_max));
  
  // Calculate gradient position
  let gradient_pos = pos / gradient.global_scale - gradient.center_radius.xy;
  
  var final_color: vec4<f32>;
  
  if (gradient.supersampling != 0u) {
    // 4x supersampling for anti-aliasing (matches desktop)
    let dpos = 0.25 * fwidth(gradient_pos);
    let color1 = get_gradient_color_at(gradient_pos + vec2<f32>(-dpos.x, -dpos.y));
    let color2 = get_gradient_color_at(gradient_pos + vec2<f32>(-dpos.x,  dpos.y));
    let color3 = get_gradient_color_at(gradient_pos + vec2<f32>( dpos.x, -dpos.y));
    let color4 = get_gradient_color_at(gradient_pos + vec2<f32>( dpos.x,  dpos.y));
    
    final_color = (color1 + color2 + color3 + color4) * 0.25;
  } else {
    final_color = get_gradient_color_at(gradient_pos);
  }
  
  return final_color * alpha;
}