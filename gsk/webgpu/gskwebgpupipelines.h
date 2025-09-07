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
#include <glib-object.h>
#include <emscripten/html5_webgpu.h>

G_BEGIN_DECLS

#define GSK_TYPE_WEBGPU_PIPELINES (gsk_webgpu_pipelines_get_type ())

G_DECLARE_FINAL_TYPE (GskWebGPUPipelines, gsk_webgpu_pipelines, GSK, WEBGPU_PIPELINES, GObject)

/**
 * GskWebGPUPipelineType:
 * @GSK_WEBGPU_PIPELINE_COLOR: Solid color rendering
 * @GSK_WEBGPU_PIPELINE_TEXTURE: Texture sampling
 * @GSK_WEBGPU_PIPELINE_LINEAR_GRADIENT: Linear gradient rendering
 * @GSK_WEBGPU_PIPELINE_RADIAL_GRADIENT: Radial gradient rendering
 * @GSK_WEBGPU_PIPELINE_CONIC_GRADIENT: Conic gradient rendering
 * @GSK_WEBGPU_PIPELINE_BORDER: Border rendering
 * @GSK_WEBGPU_PIPELINE_ROUNDED_RECT: Rounded rectangle clipping
 * @GSK_WEBGPU_PIPELINE_TEXT: Text rendering with SDF
 * @GSK_WEBGPU_PIPELINE_BLUR: Gaussian blur post-processing
 * @GSK_WEBGPU_PIPELINE_COLOR_MATRIX: Color transformation
 * @GSK_WEBGPU_PIPELINE_BLEND: Blending operations
 * @GSK_WEBGPU_PIPELINE_SHADOW: Shadow rendering
 *
 * Types of rendering pipelines available in the WebGPU renderer.
 */
typedef enum {
  GSK_WEBGPU_PIPELINE_COLOR,
  GSK_WEBGPU_PIPELINE_TEXTURE,
  GSK_WEBGPU_PIPELINE_LINEAR_GRADIENT,
  GSK_WEBGPU_PIPELINE_RADIAL_GRADIENT,
  GSK_WEBGPU_PIPELINE_CONIC_GRADIENT,
  GSK_WEBGPU_PIPELINE_BORDER,
  GSK_WEBGPU_PIPELINE_ROUNDED_RECT,
  GSK_WEBGPU_PIPELINE_TEXT,
  GSK_WEBGPU_PIPELINE_BLUR,
  GSK_WEBGPU_PIPELINE_COLOR_MATRIX,
  GSK_WEBGPU_PIPELINE_BLEND,
  GSK_WEBGPU_PIPELINE_SHADOW,
  GSK_WEBGPU_PIPELINE_COUNT
} GskWebGPUPipelineType;

/**
 * GskWebGPUComputePipelineType:
 * @GSK_WEBGPU_COMPUTE_BLUR_H: Horizontal blur pass
 * @GSK_WEBGPU_COMPUTE_BLUR_V: Vertical blur pass
 * @GSK_WEBGPU_COMPUTE_COLOR_MATRIX: Color matrix transformation
 * @GSK_WEBGPU_COMPUTE_SCALE: Image scaling/filtering
 *
 * Types of compute pipelines for GPU-accelerated image processing.
 */
typedef enum {
  GSK_WEBGPU_COMPUTE_BLUR_H,
  GSK_WEBGPU_COMPUTE_BLUR_V,
  GSK_WEBGPU_COMPUTE_COLOR_MATRIX,
  GSK_WEBGPU_COMPUTE_SCALE,
  GSK_WEBGPU_COMPUTE_COUNT
} GskWebGPUComputePipelineType;

GskWebGPUPipelines *gsk_webgpu_pipelines_new               (GskWebGPUDevice        *device);

gboolean            gsk_webgpu_pipelines_init              (GskWebGPUPipelines     *pipelines,
                                                             WGPUTextureFormat       surface_format,
                                                             GError                **error);

WGPURenderPipeline  gsk_webgpu_pipelines_get_render       (GskWebGPUPipelines     *pipelines,
                                                             GskWebGPUPipelineType   type);

WGPUComputePipeline gsk_webgpu_pipelines_get_compute      (GskWebGPUPipelines     *pipelines,
                                                             GskWebGPUComputePipelineType type);

WGPUBindGroupLayout gsk_webgpu_pipelines_get_bind_layout  (GskWebGPUPipelines     *pipelines,
                                                             GskWebGPUPipelineType   type,
                                                             uint32_t                group);

/* Vertex buffer creation helpers */
WGPUBuffer          gsk_webgpu_pipelines_create_vertex_buffer (GskWebGPUPipelines  *pipelines,
                                                                const float         *vertices,
                                                                size_t               vertex_count);

WGPUBuffer          gsk_webgpu_pipelines_create_index_buffer  (GskWebGPUPipelines  *pipelines,
                                                                const uint16_t      *indices,
                                                                size_t               index_count);

/* Uniform buffer helpers */
WGPUBuffer          gsk_webgpu_pipelines_create_uniform_buffer (GskWebGPUPipelines *pipelines,
                                                                 const void         *data,
                                                                 size_t              size);

void                gsk_webgpu_pipelines_update_uniform_buffer (GskWebGPUPipelines *pipelines,
                                                                 WGPUBuffer          buffer,
                                                                 const void         *data,
                                                                 size_t              size);

G_END_DECLS