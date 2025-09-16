/*
 * Box shadow shader - Complete WGSL implementation
 * Equivalent to gskgpuboxshadow.glsl from desktop GPU renderer  
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

struct BoxShadowUniforms {
  // Shadow rectangle and rounded corners
  shadow_rect: vec4<f32>,        // x, y, width, height
  shadow_corners: vec4<f32>,     // corner radii
  
  // Blur radius and spread
  blur_radius: f32,
  spread: f32,
  
  // Shadow offset
  offset: vec2<f32>,
  
  // Shadow color
  color: vec4<f32>,
  
  // Inset flag (0 = outset, 1 = inset)
  inset: u32,
  
  // Scale factor  
  global_scale: f32,
  
  _padding: array<f32, 2>,
}

@group(1) @binding(0) var<uniform> shadow: BoxShadowUniforms;

// Analytical box shadow calculation (matches desktop implementation)
fn box_shadow_coverage(pos: vec2<f32>) -> f32 {
  let shadow_pos = pos - shadow.offset;
  let rect = shadow.shadow_rect;
  let corners = shadow.shadow_corners;
  
  // Calculate distance to rounded rectangle
  let center = rect.xy + rect.zw * 0.5;
  let half_size = rect.zw * 0.5 + vec2<f32>(shadow.spread);
  let local_pos = abs(shadow_pos - center) - half_size;
  
  // Select corner radius based on position
  var corner_radius: f32;
  if (local_pos.x > 0.0 && local_pos.y > 0.0) {
    corner_radius = corners.z + shadow.spread; // bottom-right
  } else if (local_pos.x > 0.0) {
    corner_radius = corners.y + shadow.spread; // top-right
  } else if (local_pos.y > 0.0) {
    corner_radius = corners.w + shadow.spread; // bottom-left
  } else {
    corner_radius = corners.x + shadow.spread; // top-left
  }
  
  // SDF distance to rounded rectangle
  let distance = length(max(local_pos - vec2<f32>(corner_radius), vec2<f32>(0.0))) +
                 min(max(local_pos.x - corner_radius, local_pos.y - corner_radius), 0.0);
  
  // Apply gaussian blur
  let sigma = shadow.blur_radius / 3.0;  // 3-sigma rule
  if (sigma <= 0.0) {
    return select(0.0, 1.0, distance <= 0.0);
  }
  
  // Gaussian falloff for blur
  let blur_factor = exp(-(distance * distance) / (2.0 * sigma * sigma));
  
  if (shadow.inset != 0u) {
    // Inset shadow - inside the shape
    let inside_coverage = rounded_rect_coverage(shadow_pos, rect, corners);
    return inside_coverage * (1.0 - blur_factor);
  } else {
    // Outset shadow - outside the shape
    let outside_coverage = 1.0 - rounded_rect_coverage(shadow_pos, rect, corners);
    return outside_coverage * blur_factor;
  }
}

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  let coverage = box_shadow_coverage(input.world_pos);
  return shadow.color * coverage;
}