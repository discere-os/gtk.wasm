/*
 * Common WGSL infrastructure for GTK WebGPU renderer
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

// Vertex input layout used across all GTK shaders
struct VertexInput {
  @location(0) position: vec2<f32>,
  @location(1) tex_coord: vec2<f32>,
  @location(2) color: vec4<f32>,
}

struct VertexOutput {
  @builtin(position) position: vec4<f32>,
  @location(0) tex_coord: vec2<f32>,
  @location(1) color: vec4<f32>,
  @location(2) world_pos: vec2<f32>,
}

// Global uniforms for coordinate transformations
struct Uniforms {
  mvp_matrix: mat4x4<f32>,
  viewport_size: vec2<f32>,
  time: f32,
  _padding: f32,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

// Common utility functions for GTK rendering

// SDF functions for rounded rectangles
fn sdf_rounded_rect(p: vec2<f32>, size: vec2<f32>, radius: vec4<f32>) -> f32 {
  let r = select(select(radius.xy, radius.zw, p.x > 0.0), select(radius.wz, radius.yx, p.x > 0.0), p.y > 0.0);
  let q = abs(p) - size + r.x;
  return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r.x;
}

// Premultiplied alpha blending
fn premultiply_alpha(color: vec4<f32>) -> vec4<f32> {
  return vec4<f32>(color.rgb * color.a, color.a);
}

// sRGB conversion functions
fn linear_to_srgb(linear: vec3<f32>) -> vec3<f32> {
  let cutoff = linear < vec3<f32>(0.0031308);
  let higher = vec3<f32>(1.055) * pow(linear, vec3<f32>(1.0 / 2.4)) - vec3<f32>(0.055);
  let lower = linear * 12.92;
  return select(higher, lower, cutoff);
}

fn srgb_to_linear(srgb: vec3<f32>) -> vec3<f32> {
  let cutoff = srgb < vec3<f32>(0.04045);
  let higher = pow((srgb + vec3<f32>(0.055)) / vec3<f32>(1.055), vec3<f32>(2.4));
  let lower = srgb / 12.92;
  return select(higher, lower, cutoff);
}

// Gaussian blur weights
fn gaussian_weight(x: f32, sigma: f32) -> f32 {
  return exp(-(x * x) / (2.0 * sigma * sigma)) / (sigma * sqrt(2.0 * 3.14159265));
}