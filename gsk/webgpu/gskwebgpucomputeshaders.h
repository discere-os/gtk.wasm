/*
 * Copyright © 2025 Superstruct Ltd, New Zealand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gskwebgpudevice.h"
#include <emscripten/html5_webgpu.h>
#include <glib.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_COMPUTE_SHADERS (gsk_webgpu_compute_shaders_get_type ())

G_DECLARE_FINAL_TYPE (GskWebGPUComputeShaders, gsk_webgpu_compute_shaders, GSK, WEBGPU_COMPUTE_SHADERS, GObject)

/**
 * GskWebGPUComputeOperation:
 * @GSK_WEBGPU_COMPUTE_BLUR: Gaussian blur with separable passes
 * @GSK_WEBGPU_COMPUTE_COLOR_MATRIX: Color transformation matrix
 * @GSK_WEBGPU_COMPUTE_IMAGE_SCALE: High-quality image scaling
 * @GSK_WEBGPU_COMPUTE_EDGE_DETECTION: Edge detection filters
 * @GSK_WEBGPU_COMPUTE_CONVOLUTION: Generic convolution kernel
 * @GSK_WEBGPU_COMPUTE_TONE_MAPPING: HDR tone mapping
 * @GSK_WEBGPU_COMPUTE_HISTOGRAM: Image histogram calculation
 *
 * Types of compute shader operations available for image processing.
 */
typedef enum {
  GSK_WEBGPU_COMPUTE_BLUR,
  GSK_WEBGPU_COMPUTE_COLOR_MATRIX,
  GSK_WEBGPU_COMPUTE_IMAGE_SCALE,
  GSK_WEBGPU_COMPUTE_EDGE_DETECTION,
  GSK_WEBGPU_COMPUTE_CONVOLUTION,
  GSK_WEBGPU_COMPUTE_TONE_MAPPING,
  GSK_WEBGPU_COMPUTE_HISTOGRAM,
  GSK_WEBGPU_COMPUTE_COUNT
} GskWebGPUComputeOperation;

/**
 * GskWebGPUBlurParams:
 * @radius: blur radius in pixels
 * @sigma: Gaussian sigma parameter
 * @quality: blur quality (0-3, higher = better quality)
 *
 * Parameters for Gaussian blur compute shader.
 */
typedef struct {
  float radius;
  float sigma;
  uint32_t quality;
} GskWebGPUBlurParams;

/**
 * GskWebGPUColorMatrixParams:
 * @matrix: 4x4 color transformation matrix
 * @offset: color offset vector
 * @preserve_alpha: whether to preserve alpha channel
 *
 * Parameters for color matrix transformation.
 */
typedef struct {
  float matrix[16]; /* Column-major 4x4 matrix */
  float offset[4];
  gboolean preserve_alpha;
} GskWebGPUColorMatrixParams;

/**
 * GskWebGPUScaleParams:
 * @src_size: source texture dimensions
 * @dst_size: destination texture dimensions
 * @filter_mode: scaling filter (0=nearest, 1=linear, 2=cubic)
 * @wrap_mode: texture wrap mode for out-of-bounds sampling
 *
 * Parameters for high-quality image scaling.
 */
typedef struct {
  uint32_t src_size[2];
  uint32_t dst_size[2];
  uint32_t filter_mode;
  uint32_t wrap_mode;
} GskWebGPUScaleParams;

GskWebGPUComputeShaders *gsk_webgpu_compute_shaders_new        (GskWebGPUDevice *device);

gboolean                 gsk_webgpu_compute_shaders_init       (GskWebGPUComputeShaders *shaders,
                                                                 GError                 **error);

/* High-level compute operations */
gboolean                 gsk_webgpu_compute_blur               (GskWebGPUComputeShaders   *shaders,
                                                                 WGPUTexture                input_texture,
                                                                 WGPUTexture                output_texture,
                                                                 const GskWebGPUBlurParams *params);

gboolean                 gsk_webgpu_compute_color_matrix       (GskWebGPUComputeShaders         *shaders,
                                                                 WGPUTexture                      input_texture,
                                                                 WGPUTexture                      output_texture,
                                                                 const GskWebGPUColorMatrixParams *params);

gboolean                 gsk_webgpu_compute_scale_image        (GskWebGPUComputeShaders       *shaders,
                                                                 WGPUTexture                    input_texture,
                                                                 WGPUTexture                    output_texture,
                                                                 const GskWebGPUScaleParams    *params);

/* Advanced image processing */
gboolean                 gsk_webgpu_compute_edge_detection     (GskWebGPUComputeShaders *shaders,
                                                                 WGPUTexture              input_texture,
                                                                 WGPUTexture              output_texture,
                                                                 float                    threshold);

gboolean                 gsk_webgpu_compute_convolution        (GskWebGPUComputeShaders *shaders,
                                                                 WGPUTexture              input_texture,
                                                                 WGPUTexture              output_texture,
                                                                 const float             *kernel,
                                                                 uint32_t                 kernel_size);

/* Utility functions */
WGPUComputePipeline      gsk_webgpu_compute_shaders_get_pipeline (GskWebGPUComputeShaders   *shaders,
                                                                   GskWebGPUComputeOperation  operation);

WGPUBindGroupLayout      gsk_webgpu_compute_shaders_get_layout    (GskWebGPUComputeShaders   *shaders,
                                                                   GskWebGPUComputeOperation  operation);

/* Performance monitoring */
double                   gsk_webgpu_compute_shaders_get_last_dispatch_time (GskWebGPUComputeShaders *shaders);
uint64_t                 gsk_webgpu_compute_shaders_get_total_dispatches   (GskWebGPUComputeShaders *shaders);

G_END_DECLS