/*
 * Conic gradient shader - Complete WGSL implementation
 * Equivalent to gskgpuconicgradient.glsl from desktop GPU renderer
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

struct ConicGradientUniforms {
  // Rectangle bounds
  rect: vec4<f32>,
  
  // 7 color stops
  colors: array<vec4<f32>, 7>,
  offsets: array<f32, 7>,
  hints: array<f32, 7>,
  
  // Center position and angle
  center: vec2<f32>,
  angle: f32,
  
  // Variation flags
  supersampling: u32,
  repeating: u32,
  
  // Scale
  global_scale: f32,
  _padding: f32,
}

@group(1) @binding(0) var<uniform> gradient: ConicGradientUniforms;

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

fn get_gradient_color(offset: f32) -> vec4<f32> {
  // Same 7-way gradient interpolation as radial
  var color: vec4<f32>;
  var f: f32;
  
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

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  // Rectangle coverage
  let rect_min = gradient.rect.xy;
  let rect_max = gradient.rect.xy + gradient.rect.zw;
  let pos = input.world_pos;
  
  let alpha = select(0.0, 1.0, 
    all(pos >= rect_min) && all(pos <= rect_max));
  
  // Calculate conic gradient angle
  let gradient_pos = pos / gradient.global_scale - gradient.center;
  let angle = atan2(gradient_pos.y, gradient_pos.x) + gradient.angle;
  let pi = 3.14159265359;
  var offset = (angle + pi) / (2.0 * pi);  // Normalize to [0,1]
  
  if (gradient.repeating != 0u) {
    offset = fract(offset);
  } else {
    offset = clamp(offset, 0.0, 1.0);
  }
  
  var final_color: vec4<f32>;
  
  if (gradient.supersampling != 0u) {
    // 4x supersampling for smooth conic gradients
    let dpos = 0.25 * fwidth(gradient_pos);
    let angle1 = atan2(gradient_pos.y - dpos.y, gradient_pos.x - dpos.x) + gradient.angle;
    let angle2 = atan2(gradient_pos.y + dpos.y, gradient_pos.x - dpos.x) + gradient.angle;
    let angle3 = atan2(gradient_pos.y - dpos.y, gradient_pos.x + dpos.x) + gradient.angle;
    let angle4 = atan2(gradient_pos.y + dpos.y, gradient_pos.x + dpos.x) + gradient.angle;
    
    let offset1 = (angle1 + pi) / (2.0 * pi);
    let offset2 = (angle2 + pi) / (2.0 * pi);
    let offset3 = (angle3 + pi) / (2.0 * pi);
    let offset4 = (angle4 + pi) / (2.0 * pi);
    
    let color1 = get_gradient_color(clamp(offset1, 0.0, 1.0));
    let color2 = get_gradient_color(clamp(offset2, 0.0, 1.0));
    let color3 = get_gradient_color(clamp(offset3, 0.0, 1.0));
    let color4 = get_gradient_color(clamp(offset4, 0.0, 1.0));
    
    final_color = (color1 + color2 + color3 + color4) * 0.25;
  } else {
    final_color = get_gradient_color(offset);
  }
  
  return final_color * alpha;
}