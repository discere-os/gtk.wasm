/*
 * Standard 2D vertex shader for GTK WebGPU rendering
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

@vertex fn vs_main(input: VertexInput) -> VertexOutput {
  var output: VertexOutput;
  output.position = uniforms.mvp_matrix * vec4<f32>(input.position, 0.0, 1.0);
  output.tex_coord = input.tex_coord;
  output.color = input.color;
  output.world_pos = input.position;
  return output;
}