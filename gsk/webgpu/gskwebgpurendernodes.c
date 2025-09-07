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

#include "gskwebgpurendererprivate.h"
#include "gskwebgpupipelines.h"
#include "gskwebgpushaders.h"

#include "gskrendernodeprivate.h"
#include "gskdebugprivate.h"

#include <graphene.h>

/**
 * Main render node dispatcher - maps GSK render nodes to WebGPU operations
 */
void
gsk_webgpu_renderer_render_node (GskWebGPURenderer *renderer,
                                  GskRenderNode     *node)
{
  if (!node)
    return;

  /* Get the render node type and dispatch to appropriate handler */
  GskRenderNodeType node_type = gsk_render_node_get_node_type (node);

  switch (node_type)
    {
    case GSK_CONTAINER_NODE:
      gsk_webgpu_renderer_render_container_node (renderer, node);
      break;
      
    case GSK_CAIRO_NODE:
      gsk_webgpu_renderer_render_cairo_node (renderer, node);
      break;
      
    case GSK_COLOR_NODE:
      gsk_webgpu_renderer_render_color_node (renderer, node);
      break;
      
    case GSK_TEXTURE_NODE:
      gsk_webgpu_renderer_render_texture_node (renderer, node);
      break;
      
    case GSK_LINEAR_GRADIENT_NODE:
      gsk_webgpu_renderer_render_linear_gradient_node (renderer, node);
      break;
      
    case GSK_RADIAL_GRADIENT_NODE:
      gsk_webgpu_renderer_render_radial_gradient_node (renderer, node);
      break;
      
    case GSK_CONIC_GRADIENT_NODE:
      gsk_webgpu_renderer_render_conic_gradient_node (renderer, node);
      break;
      
    case GSK_BORDER_NODE:
      gsk_webgpu_renderer_render_border_node (renderer, node);
      break;
      
    case GSK_INSET_SHADOW_NODE:
      gsk_webgpu_renderer_render_inset_shadow_node (renderer, node);
      break;
      
    case GSK_OUTSET_SHADOW_NODE:
      gsk_webgpu_renderer_render_outset_shadow_node (renderer, node);
      break;
      
    case GSK_TRANSFORM_NODE:
      gsk_webgpu_renderer_render_transform_node (renderer, node);
      break;
      
    case GSK_OPACITY_NODE:
      gsk_webgpu_renderer_render_opacity_node (renderer, node);
      break;
      
    case GSK_COLOR_MATRIX_NODE:
      gsk_webgpu_renderer_render_color_matrix_node (renderer, node);
      break;
      
    case GSK_REPEAT_NODE:
      gsk_webgpu_renderer_render_repeat_node (renderer, node);
      break;
      
    case GSK_CLIP_NODE:
      gsk_webgpu_renderer_render_clip_node (renderer, node);
      break;
      
    case GSK_ROUNDED_CLIP_NODE:
      gsk_webgpu_renderer_render_rounded_clip_node (renderer, node);
      break;
      
    case GSK_SHADOW_NODE:
      gsk_webgpu_renderer_render_shadow_node (renderer, node);
      break;
      
    case GSK_BLEND_NODE:
      gsk_webgpu_renderer_render_blend_node (renderer, node);
      break;
      
    case GSK_CROSS_FADE_NODE:
      gsk_webgpu_renderer_render_cross_fade_node (renderer, node);
      break;
      
    case GSK_TEXT_NODE:
      gsk_webgpu_renderer_render_text_node (renderer, node);
      break;
      
    case GSK_BLUR_NODE:
      gsk_webgpu_renderer_render_blur_node (renderer, node);
      break;
      
    case GSK_DEBUG_NODE:
      gsk_webgpu_renderer_render_debug_node (renderer, node);
      break;
      
    case GSK_GL_SHADER_NODE:
      gsk_webgpu_renderer_render_gl_shader_node (renderer, node);
      break;

    /* GTK 4.10+ nodes */
    case GSK_TEXTURE_SCALE_NODE:
      gsk_webgpu_renderer_render_texture_scale_node (renderer, node);
      break;
      
    case GSK_MASK_NODE:
      gsk_webgpu_renderer_render_mask_node (renderer, node);
      break;

    /* GTK 4.14+ nodes */
    case GSK_STROKE_NODE:
      gsk_webgpu_renderer_render_stroke_node (renderer, node);
      break;
      
    case GSK_FILL_NODE:
      gsk_webgpu_renderer_render_fill_node (renderer, node);
      break;
      
    case GSK_SUBSURFACE_NODE:
      gsk_webgpu_renderer_render_subsurface_node (renderer, node);
      break;

    case GSK_NOT_A_RENDER_NODE:
    default:
      g_warning ("Unknown render node type: %d", node_type);
      break;
    }
}

/* Container node - renders all children in sequence */
void
gsk_webgpu_renderer_render_container_node (GskWebGPURenderer *renderer,
                                            GskRenderNode     *node)
{
  guint n_children = gsk_container_node_get_n_children (node);
  
  for (guint i = 0; i < n_children; i++)
    {
      GskRenderNode *child = gsk_container_node_get_child (node, i);
      gsk_webgpu_renderer_render_node (renderer, child);
    }
}

/* Cairo node - convert Cairo surface to WebGPU texture */
void
gsk_webgpu_renderer_render_cairo_node (GskWebGPURenderer *renderer,
                                        GskRenderNode     *node)
{
  cairo_surface_t *surface = gsk_cairo_node_get_surface (node);
  const graphene_rect_t *bounds = gsk_render_node_get_bounds (node);
  
  if (!surface || cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
    return;

  /* Convert Cairo surface to WebGPU texture */
  cairo_surface_flush (surface);
  
  int width = cairo_image_surface_get_width (surface);
  int height = cairo_image_surface_get_height (surface);
  unsigned char *data = cairo_image_surface_get_data (surface);
  
  if (!data || width <= 0 || height <= 0)
    return;

  /* Create WebGPU texture from Cairo surface data */
  WGPUTextureDescriptor texture_desc = {
    .label = "Cairo Surface Texture",
    .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
    .dimension = WGPUTextureDimension_2D,
    .size = { width, height, 1 },
    .format = WGPUTextureFormat_RGBA8Unorm,
    .mipLevelCount = 1,
    .sampleCount = 1
  };

  WGPUTexture texture = wgpuDeviceCreateTexture (
    gsk_webgpu_renderer_get_device (renderer), &texture_desc);

  /* Upload Cairo data to texture */
  gsk_webgpu_device_write_texture (renderer->priv->device, texture, data, 
                                    width * height * 4, width, height, 
                                    WGPUTextureFormat_RGBA8Unorm);

  /* Render textured quad */
  float vertices[] = {
    bounds->origin.x, bounds->origin.y, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    bounds->origin.x + bounds->size.width, bounds->origin.y, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    bounds->origin.x + bounds->size.width, bounds->origin.y + bounds->size.height, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    bounds->origin.x, bounds->origin.y + bounds->size.height, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
  };

  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  WGPUBuffer vertex_buffer = gsk_webgpu_pipelines_create_vertex_buffer (
    renderer->priv->pipelines, vertices, 4);
  WGPUBuffer index_buffer = gsk_webgpu_pipelines_create_index_buffer (
    renderer->priv->pipelines, indices, 6);

  /* Set up texture binding */
  WGPUSampler sampler = gsk_webgpu_device_create_sampler (
    renderer->priv->device, "Cairo Sampler", 
    WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear);
  
  WGPUTextureView texture_view = wgpuTextureCreateView (texture, NULL);

  WGPUBindGroupEntry entries[] = {
    { .binding = 0, .sampler = sampler },
    { .binding = 1, .textureView = texture_view }
  };

  WGPUBindGroupLayout layout = gsk_webgpu_pipelines_get_bind_layout (
    renderer->priv->pipelines, GSK_WEBGPU_PIPELINE_TEXTURE, 1);
    
  WGPUBindGroup bind_group = gsk_webgpu_device_create_bind_group (
    renderer->priv->device, "Cairo Texture Bind Group", layout, 2, entries);

  /* Render */
  WGPURenderPipeline pipeline = gsk_webgpu_pipelines_get_render (
    renderer->priv->pipelines, GSK_WEBGPU_PIPELINE_TEXTURE);
    
  wgpuRenderPassEncoderSetPipeline (renderer->priv->render_pass, pipeline);
  wgpuRenderPassEncoderSetBindGroup (renderer->priv->render_pass, 1, bind_group, 0, NULL);
  wgpuRenderPassEncoderSetVertexBuffer (renderer->priv->render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer (renderer->priv->render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed (renderer->priv->render_pass, 6, 1, 0, 0, 0);

  /* Cleanup */
  wgpuTextureViewRelease (texture_view);
  wgpuTextureRelease (texture);
  wgpuSamplerRelease (sampler);
  wgpuBindGroupRelease (bind_group);
  wgpuBufferRelease (vertex_buffer);
  wgpuBufferRelease (index_buffer);
}

/* Color node - renders a solid color rectangle */
void
gsk_webgpu_renderer_render_color_node (GskWebGPURenderer *renderer,
                                        GskRenderNode     *node)
{
  const GdkRGBA *color = gsk_color_node_get_color (node);
  const graphene_rect_t *bounds = gsk_render_node_get_bounds (node);

  /* Create vertices for colored rectangle */
  float vertices[] = {
    bounds->origin.x, bounds->origin.y, 0.0f, 0.0f, 
    color->red, color->green, color->blue, color->alpha,
    
    bounds->origin.x + bounds->size.width, bounds->origin.y, 1.0f, 0.0f,
    color->red, color->green, color->blue, color->alpha,
    
    bounds->origin.x + bounds->size.width, bounds->origin.y + bounds->size.height, 1.0f, 1.0f,
    color->red, color->green, color->blue, color->alpha,
    
    bounds->origin.x, bounds->origin.y + bounds->size.height, 0.0f, 1.0f,
    color->red, color->green, color->blue, color->alpha
  };

  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  WGPUBuffer vertex_buffer = gsk_webgpu_pipelines_create_vertex_buffer (
    renderer->priv->pipelines, vertices, 4);
  WGPUBuffer index_buffer = gsk_webgpu_pipelines_create_index_buffer (
    renderer->priv->pipelines, indices, 6);

  /* Render colored rectangle */
  WGPURenderPipeline pipeline = gsk_webgpu_pipelines_get_render (
    renderer->priv->pipelines, GSK_WEBGPU_PIPELINE_COLOR);
    
  wgpuRenderPassEncoderSetPipeline (renderer->priv->render_pass, pipeline);
  wgpuRenderPassEncoderSetVertexBuffer (renderer->priv->render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer (renderer->priv->render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed (renderer->priv->render_pass, 6, 1, 0, 0, 0);

  /* Cleanup */
  wgpuBufferRelease (vertex_buffer);
  wgpuBufferRelease (index_buffer);
}

/* Texture node - renders a GdkTexture */
void
gsk_webgpu_renderer_render_texture_node (GskWebGPURenderer *renderer,
                                          GskRenderNode     *node)
{
  GdkTexture *texture = gsk_texture_node_get_texture (node);
  const graphene_rect_t *bounds = gsk_render_node_get_bounds (node);
  
  if (!texture)
    return;

  /* Convert GdkTexture to WebGPU texture */
  int width = gdk_texture_get_width (texture);
  int height = gdk_texture_get_height (texture);
  
  /* Download texture data */
  GBytes *texture_data = gdk_texture_save_to_png_bytes (texture);
  gsize data_size;
  gconstpointer data = g_bytes_get_data (texture_data, &data_size);

  /* For now, we'll need to decode PNG data - in a real implementation,
   * we'd want to handle various texture formats more efficiently */
  
  /* Create placeholder textured quad */
  float vertices[] = {
    bounds->origin.x, bounds->origin.y, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    bounds->origin.x + bounds->size.width, bounds->origin.y, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    bounds->origin.x + bounds->size.width, bounds->origin.y + bounds->size.height, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    bounds->origin.x, bounds->origin.y + bounds->size.height, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
  };

  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  WGPUBuffer vertex_buffer = gsk_webgpu_pipelines_create_vertex_buffer (
    renderer->priv->pipelines, vertices, 4);
  WGPUBuffer index_buffer = gsk_webgpu_pipelines_create_index_buffer (
    renderer->priv->pipelines, indices, 6);

  /* TODO: Create actual WebGPU texture from GdkTexture data */
  /* For now, render as a colored rectangle */
  WGPURenderPipeline pipeline = gsk_webgpu_pipelines_get_render (
    renderer->priv->pipelines, GSK_WEBGPU_PIPELINE_COLOR);
    
  wgpuRenderPassEncoderSetPipeline (renderer->priv->render_pass, pipeline);
  wgpuRenderPassEncoderSetVertexBuffer (renderer->priv->render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer (renderer->priv->render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed (renderer->priv->render_pass, 6, 1, 0, 0, 0);

  /* Cleanup */
  g_bytes_unref (texture_data);
  wgpuBufferRelease (vertex_buffer);
  wgpuBufferRelease (index_buffer);
}

/* Transform node - applies transformation matrix to child */
void
gsk_webgpu_renderer_render_transform_node (GskWebGPURenderer *renderer,
                                            GskRenderNode     *node)
{
  GskTransform *transform = gsk_transform_node_get_transform (node);
  GskRenderNode *child = gsk_transform_node_get_child (node);
  
  if (!child)
    return;

  /* TODO: Apply transformation matrix to MVP matrix in uniforms */
  /* For now, just render the child without transformation */
  gsk_webgpu_renderer_render_node (renderer, child);
}

/* Opacity node - applies opacity to child rendering */
void
gsk_webgpu_renderer_render_opacity_node (GskWebGPURenderer *renderer,
                                          GskRenderNode     *node)
{
  float opacity = gsk_opacity_node_get_opacity (node);
  GskRenderNode *child = gsk_opacity_node_get_child (node);
  
  if (!child || opacity <= 0.0f)
    return;

  /* TODO: Implement opacity by rendering child to temporary texture
   * and then blending with specified opacity */
  /* For now, just render the child */
  gsk_webgpu_renderer_render_node (renderer, child);
}

/* Text node - renders text using SDF font atlas */
void
gsk_webgpu_renderer_render_text_node (GskWebGPURenderer *renderer,
                                       GskRenderNode     *node)
{
  PangoFont *font = gsk_text_node_get_font (node);
  PangoGlyphString *glyphs = gsk_text_node_get_glyphs (node);
  const GdkRGBA *color = gsk_text_node_get_color (node);
  const graphene_point_t *offset = gsk_text_node_get_offset (node);
  
  if (!font || !glyphs || glyphs->num_glyphs == 0)
    return;

  /* TODO: Implement proper text rendering with font atlas and SDF */
  /* This would involve:
   * 1. Creating/updating font atlas texture
   * 2. Generating vertex data for each glyph
   * 3. Using text shader with SDF rendering
   */
  
  /* For now, render a placeholder colored rectangle */
  const graphene_rect_t *bounds = gsk_render_node_get_bounds (node);
  
  float vertices[] = {
    bounds->origin.x, bounds->origin.y, 0.0f, 0.0f, 
    color->red, color->green, color->blue, color->alpha,
    
    bounds->origin.x + bounds->size.width, bounds->origin.y, 1.0f, 0.0f,
    color->red, color->green, color->blue, color->alpha,
    
    bounds->origin.x + bounds->size.width, bounds->origin.y + bounds->size.height, 1.0f, 1.0f,
    color->red, color->green, color->blue, color->alpha,
    
    bounds->origin.x, bounds->origin.y + bounds->size.height, 0.0f, 1.0f,
    color->red, color->green, color->blue, color->alpha
  };

  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  WGPUBuffer vertex_buffer = gsk_webgpu_pipelines_create_vertex_buffer (
    renderer->priv->pipelines, vertices, 4);
  WGPUBuffer index_buffer = gsk_webgpu_pipelines_create_index_buffer (
    renderer->priv->pipelines, indices, 6);

  WGPURenderPipeline pipeline = gsk_webgpu_pipelines_get_render (
    renderer->priv->pipelines, GSK_WEBGPU_PIPELINE_COLOR);
    
  wgpuRenderPassEncoderSetPipeline (renderer->priv->render_pass, pipeline);
  wgpuRenderPassEncoderSetVertexBuffer (renderer->priv->render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer (renderer->priv->render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed (renderer->priv->render_pass, 6, 1, 0, 0, 0);

  wgpuBufferRelease (vertex_buffer);
  wgpuBufferRelease (index_buffer);
}

/* Stub implementations for remaining render node types */
/* These would be implemented with appropriate WebGPU shaders and rendering logic */

void gsk_webgpu_renderer_render_linear_gradient_node (GskWebGPURenderer *renderer, GskRenderNode *node)
{
  /* TODO: Implement linear gradient rendering */
  const graphene_rect_t *bounds = gsk_render_node_get_bounds (node);
  
  /* Render placeholder colored rectangle */
  float vertices[] = {
    bounds->origin.x, bounds->origin.y, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    bounds->origin.x + bounds->size.width, bounds->origin.y, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    bounds->origin.x + bounds->size.width, bounds->origin.y + bounds->size.height, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    bounds->origin.x, bounds->origin.y + bounds->size.height, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f
  };

  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  WGPUBuffer vertex_buffer = gsk_webgpu_pipelines_create_vertex_buffer (renderer->priv->pipelines, vertices, 4);
  WGPUBuffer index_buffer = gsk_webgpu_pipelines_create_index_buffer (renderer->priv->pipelines, indices, 6);

  WGPURenderPipeline pipeline = gsk_webgpu_pipelines_get_render (renderer->priv->pipelines, GSK_WEBGPU_PIPELINE_COLOR);
  wgpuRenderPassEncoderSetPipeline (renderer->priv->render_pass, pipeline);
  wgpuRenderPassEncoderSetVertexBuffer (renderer->priv->render_pass, 0, vertex_buffer, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer (renderer->priv->render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed (renderer->priv->render_pass, 6, 1, 0, 0, 0);

  wgpuBufferRelease (vertex_buffer);
  wgpuBufferRelease (index_buffer);
}

void gsk_webgpu_renderer_render_radial_gradient_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_conic_gradient_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_border_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_inset_shadow_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_outset_shadow_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_color_matrix_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_repeat_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_clip_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_rounded_clip_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_shadow_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_blend_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_cross_fade_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_blur_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_debug_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_gl_shader_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_texture_scale_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_mask_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_stroke_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_fill_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }
void gsk_webgpu_renderer_render_subsurface_node (GskWebGPURenderer *renderer, GskRenderNode *node) { /* TODO */ }