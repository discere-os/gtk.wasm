/*
 * Border rendering shader - Complete WGSL implementation  
 * Equivalent to gskgpuborder.glsl from desktop GPU renderer
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

struct BorderUniforms {
  // Border colors (top, right, bottom, left)
  border_colors: array<vec4<f32>, 4>,
  
  // Rounded rectangle definition (bounds + corner radii)
  outside_rect: vec4<f32>,      // x, y, width, height
  outside_corners: vec4<f32>,   // corner radii
  
  inside_rect: vec4<f32>,       // inner bounds  
  inside_corners: vec4<f32>,    // inner corner radii
  
  // Border widths (top, right, bottom, left)
  border_widths: vec4<f32>,
  
  // Position offset
  offset: vec2<f32>,
  
  // Scale factor
  global_scale: f32,
  
  _padding: f32,
}

@group(1) @binding(0) var<uniform> border: BorderUniforms;

// Border slice indices (matching desktop implementation)
const SLICE_TOP_LEFT = 0u;
const SLICE_TOP = 1u;
const SLICE_TOP_RIGHT = 2u;
const SLICE_RIGHT = 3u;
const SLICE_BOTTOM_RIGHT = 4u;
const SLICE_BOTTOM = 5u;
const SLICE_BOTTOM_LEFT = 6u;
const SLICE_LEFT = 7u;

// Border color indices
const TOP = 0u;
const RIGHT = 1u;
const BOTTOM = 2u;
const LEFT = 3u;

// Compute border color based on vertex index (triangle-based slicing)
fn compute_border_color(vertex_index: u32) -> vec4<f32> {
  let triangle_index = vertex_index / 3u;
  
  switch (triangle_index) {
    case 2u * SLICE_TOP_LEFT + 1u: {
      if (border.border_widths[TOP] > 0.0) {
        return border.border_colors[TOP];
      } else {
        return border.border_colors[LEFT];
      }
    }
    
    case 2u * SLICE_TOP, 2u * SLICE_TOP + 1u: {
      return border.border_colors[TOP];
    }
    
    case 2u * SLICE_TOP_RIGHT: {
      if (border.border_widths[TOP] > 0.0) {
        return border.border_colors[TOP];
      } else {
        return border.border_colors[RIGHT];
      }
    }
    
    case 2u * SLICE_TOP_RIGHT + 1u: {
      if (border.border_widths[RIGHT] > 0.0) {
        return border.border_colors[RIGHT];
      } else {
        return border.border_colors[TOP];
      }
    }
    
    case 2u * SLICE_RIGHT, 2u * SLICE_RIGHT + 1u: {
      return border.border_colors[RIGHT];
    }
    
    case 2u * SLICE_BOTTOM_RIGHT: {
      if (border.border_widths[RIGHT] > 0.0) {
        return border.border_colors[RIGHT];
      } else {
        return border.border_colors[BOTTOM];
      }
    }
    
    case 2u * SLICE_BOTTOM_RIGHT + 1u: {
      if (border.border_widths[BOTTOM] > 0.0) {
        return border.border_colors[BOTTOM];
      } else {
        return border.border_colors[RIGHT];
      }
    }
    
    case 2u * SLICE_BOTTOM, 2u * SLICE_BOTTOM + 1u: {
      return border.border_colors[BOTTOM];
    }
    
    case 2u * SLICE_BOTTOM_LEFT: {
      if (border.border_widths[BOTTOM] > 0.0) {
        return border.border_colors[BOTTOM];
      } else {
        return border.border_colors[LEFT];
      }
    }
    
    case 2u * SLICE_BOTTOM_LEFT + 1u: {
      if (border.border_widths[LEFT] > 0.0) {
        return border.border_colors[LEFT];
      } else {
        return border.border_colors[BOTTOM];
      }
    }
    
    case 2u * SLICE_LEFT, 2u * SLICE_LEFT + 1u: {
      return border.border_colors[LEFT];
    }
    
    case 2u * SLICE_TOP_LEFT: {
      if (border.border_widths[LEFT] > 0.0) {
        return border.border_colors[LEFT];
      } else {
        return border.border_colors[TOP];
      }
    }
    
    default: {
      return border.border_colors[TOP];
    }
  }
}

@vertex fn vs_main(input: VertexInput) -> VertexOutput {
  var output: VertexOutput;
  
  // Transform position 
  output.position = uniforms.mvp_matrix * vec4<f32>(input.position, 0.0, 1.0);
  output.tex_coord = input.tex_coord;
  output.world_pos = input.position;
  
  // Compute color based on border slice
  output.color = compute_border_color(input.vertex_index);
  
  return output;
}

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  let pos = input.world_pos;
  
  // Calculate outside rounded rectangle coverage
  let outside_coverage = rounded_rect_coverage(pos, border.outside_rect, border.outside_corners);
  
  // Calculate inside rounded rectangle coverage  
  let inside_coverage = rounded_rect_coverage(pos, border.inside_rect, border.inside_corners);
  
  // Border alpha = outside - inside
  let alpha = clamp(outside_coverage - inside_coverage, 0.0, 1.0);
  
  return input.color * alpha;
}

// Helper function for rounded rectangle coverage (matches desktop)
fn rounded_rect_coverage(pos: vec2<f32>, rect: vec4<f32>, corners: vec4<f32>) -> f32 {
  let center = rect.xy + rect.zw * 0.5;
  let half_size = rect.zw * 0.5;
  let local_pos = abs(pos - center) - half_size;
  
  // Select corner radius based on quadrant
  var corner_radius: f32;
  if (local_pos.x > 0.0 && local_pos.y > 0.0) {
    corner_radius = corners.z; // bottom-right
  } else if (local_pos.x > 0.0) {
    corner_radius = corners.y; // top-right  
  } else if (local_pos.y > 0.0) {
    corner_radius = corners.w; // bottom-left
  } else {
    corner_radius = corners.x; // top-left
  }
  
  let distance = length(max(local_pos - vec2<f32>(corner_radius), vec2<f32>(0.0))) + 
                 min(max(local_pos.x - corner_radius, local_pos.y - corner_radius), 0.0);
  
  return saturate(0.5 - distance);
}