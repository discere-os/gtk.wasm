/*
 * Solid color rendering fragment shader
 * Copyright © 2025 Superstruct Ltd, New Zealand  
 * Licensed under LGPL-2.1-or-later
 */

@fragment fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
  return premultiply_alpha(input.color);
}