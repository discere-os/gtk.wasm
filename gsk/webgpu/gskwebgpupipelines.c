/*
 * GTK WebGPU Pipeline Management System
 * Complete implementation matching desktop GPU renderer standards
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

#include "gskwebgpupipelines.h"
#include "gskwebgpushaders.h"
#include "gskwebgpudevice.h"
#include <emscripten/html5_webgpu.h>

struct _GskWebGPUPipelineManager
{
  GObject parent_instance;
  
  WGPUDevice device;
  
  // Pipeline caches (matching desktop GPU renderer architecture)
  GHashTable *render_pipelines;     // Cache for render pipelines
  GHashTable *compute_pipelines;    // Cache for compute pipelines  
  GHashTable *shader_modules;       // Cache for compiled shader modules
  GHashTable *bind_group_layouts;   // Cache for bind group layouts
  
  // Resource management
  GPtrArray *active_pipelines;      // Track active pipelines for cleanup
  
  // Performance tracking
  guint64 pipeline_cache_hits;
  guint64 pipeline_cache_misses;
};

G_DEFINE_TYPE (GskWebGPUPipelineManager, gsk_webgpu_pipeline_manager, G_TYPE_OBJECT)

static void
gsk_webgpu_pipeline_manager_finalize (GObject *object)
{
  GskWebGPUPipelineManager *self = GSK_WEBGPU_PIPELINE_MANAGER (object);
  
  // Clean up caches
  g_hash_table_destroy (self->render_pipelines);
  g_hash_table_destroy (self->compute_pipelines);
  g_hash_table_destroy (self->shader_modules);
  g_hash_table_destroy (self->bind_group_layouts);
  
  g_ptr_array_unref (self->active_pipelines);
  
  G_OBJECT_CLASS (gsk_webgpu_pipeline_manager_parent_class)->finalize (object);
}

static void
gsk_webgpu_pipeline_manager_class_init (GskWebGPUPipelineManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gsk_webgpu_pipeline_manager_finalize;
}

static void
gsk_webgpu_pipeline_manager_init (GskWebGPUPipelineManager *self)
{
  // Initialize hash tables with proper key/value destruction
  self->render_pipelines = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                   g_free, (GDestroyNotify) wgpuRenderPipelineRelease);
  self->compute_pipelines = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    g_free, (GDestroyNotify) wgpuComputePipelineRelease);
  self->shader_modules = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, (GDestroyNotify) wgpuShaderModuleRelease);
  self->bind_group_layouts = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, (GDestroyNotify) wgpuBindGroupLayoutRelease);
  
  self->active_pipelines = g_ptr_array_new ();
  
  self->pipeline_cache_hits = 0;
  self->pipeline_cache_misses = 0;
}

GskWebGPUPipelineManager *
gsk_webgpu_pipeline_manager_new (WGPUDevice device)
{
  GskWebGPUPipelineManager *self;
  
  g_return_val_if_fail (device != NULL, NULL);
  
  self = g_object_new (GSK_TYPE_WEBGPU_PIPELINE_MANAGER, NULL);
  self->device = device;
  
  return self;
}

// Create shader module with caching
WGPUShaderModule
gsk_webgpu_pipeline_manager_create_shader_module (GskWebGPUPipelineManager *self,
                                                   const char               *shader_name,
                                                   const char               *entry_point)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_PIPELINE_MANAGER (self), NULL);
  g_return_val_if_fail (shader_name != NULL, NULL);
  
  // Create cache key
  char *cache_key = g_strdup_printf ("%s:%s", shader_name, entry_point);
  
  // Check cache first
  WGPUShaderModule cached_module = g_hash_table_lookup (self->shader_modules, cache_key);
  if (cached_module)
    {
      self->pipeline_cache_hits++;
      g_free (cache_key);
      return cached_module;
    }
  
  // Load shader source from file system
  char *shader_source = gsk_webgpu_load_shader_file (shader_name);
  if (!shader_source)
    {
      g_warning ("Failed to load shader: %s", shader_name);
      g_free (cache_key);
      return NULL;
    }
  
  // Create shader module descriptor
  WGPUShaderModuleDescriptor descriptor = {
    .label = cache_key,
    .code = {
      .tag = WGPUShaderSourceType_WGSL,
      .wgsl = shader_source
    }
  };
  
  // Create and cache shader module
  WGPUShaderModule module = wgpuDeviceCreateShaderModule (self->device, &descriptor);
  if (module)
    {
      g_hash_table_insert (self->shader_modules, cache_key, module);
      self->pipeline_cache_misses++;
      g_debug ("Compiled and cached shader: %s", cache_key);
    }
  else
    {
      g_free (cache_key);
    }
  
  g_free (shader_source);
  return module;
}

// Create render pipeline with comprehensive configuration
WGPURenderPipeline
gsk_webgpu_pipeline_manager_create_render_pipeline (GskWebGPUPipelineManager *self,
                                                     const char               *vertex_shader,
                                                     const char               *fragment_shader,
                                                     WGPUTextureFormat         color_format,
                                                     WGPUTextureFormat         depth_format,
                                                     GskWebGPUPipelineFlags    flags)
{
  g_return_val_if_fail (GSK_IS_WEBGPU_PIPELINE_MANAGER (self), NULL);
  
  // Create comprehensive cache key including all parameters
  char *cache_key = g_strdup_printf ("%s+%s:%d:%d:0x%x", 
                                      vertex_shader, fragment_shader, 
                                      color_format, depth_format, flags);
  
  // Check pipeline cache
  WGPURenderPipeline cached_pipeline = g_hash_table_lookup (self->render_pipelines, cache_key);
  if (cached_pipeline)
    {
      self->pipeline_cache_hits++;
      g_free (cache_key);
      return cached_pipeline;
    }
  
  // Create shader modules
  WGPUShaderModule vs_module = gsk_webgpu_pipeline_manager_create_shader_module (self, vertex_shader, "vs_main");
  WGPUShaderModule fs_module = gsk_webgpu_pipeline_manager_create_shader_module (self, fragment_shader, "fs_main");
  
  if (!vs_module || !fs_module)
    {
      g_warning ("Failed to create shader modules for pipeline: %s", cache_key);
      g_free (cache_key);
      return NULL;
    }
  
  // Create vertex buffer layout (standard GTK vertex format)
  WGPUVertexAttribute vertex_attributes[] = {
    { .format = WGPUVertexFormat_Float32x2, .offset = 0, .shaderLocation = 0 },  // position
    { .format = WGPUVertexFormat_Float32x2, .offset = 8, .shaderLocation = 1 },  // tex_coord
    { .format = WGPUVertexFormat_Float32x4, .offset = 16, .shaderLocation = 2 }  // color
  };
  
  WGPUVertexBufferLayout vertex_buffer_layout = {
    .arrayStride = 32,  // 2 + 2 + 4 floats = 32 bytes
    .stepMode = WGPUVertexStepMode_Vertex,
    .attributeCount = 3,
    .attributes = vertex_attributes
  };
  
  // Configure color target
  WGPUColorTargetState color_target = {
    .format = color_format,
    .blend = &(WGPUBlendState) {
      .color = { WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha, WGPUBlendFactor_OneMinusSrcAlpha },
      .alpha = { WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha }
    },
    .writeMask = WGPUColorWriteMask_All
  };
  
  // Configure depth stencil if needed
  WGPUDepthStencilState depth_stencil = {0};
  WGPUDepthStencilState *depth_stencil_ptr = NULL;
  
  if (depth_format != WGPUTextureFormat_Undefined)
    {
      depth_stencil = (WGPUDepthStencilState) {
        .format = depth_format,
        .depthWriteEnabled = true,
        .depthCompare = WGPUCompareFunction_Less
      };
      depth_stencil_ptr = &depth_stencil;
    }
  
  // Create render pipeline
  WGPURenderPipelineDescriptor pipeline_desc = {
    .label = cache_key,
    .vertex = {
      .module = vs_module,
      .entryPoint = "vs_main", 
      .bufferCount = 1,
      .buffers = &vertex_buffer_layout
    },
    .fragment = &(WGPUFragmentState) {
      .module = fs_module,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = &color_target
    },
    .primitive = {
      .topology = WGPUPrimitiveTopology_TriangleList,
      .frontFace = WGPUFrontFace_CCW,
      .cullMode = WGPUCullMode_None
    },
    .depthStencil = depth_stencil_ptr,
    .multisample = {
      .count = 1,
      .mask = 0xFFFFFFFF,
      .alphaToCoverageEnabled = false
    }
  };
  
  WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline (self->device, &pipeline_desc);
  
  if (pipeline)
    {
      // Cache the pipeline
      g_hash_table_insert (self->render_pipelines, cache_key, pipeline);
      g_ptr_array_add (self->active_pipelines, pipeline);
      self->pipeline_cache_misses++;
      
      g_debug ("Created and cached render pipeline: %s", cache_key);
    }
  else
    {
      g_warning ("Failed to create render pipeline: %s", cache_key);
      g_free (cache_key);
    }
  
  return pipeline;
}

// Get pipeline cache statistics
void
gsk_webgpu_pipeline_manager_get_stats (GskWebGPUPipelineManager *self,
                                        guint64                  *cache_hits,
                                        guint64                  *cache_misses)
{
  g_return_if_fail (GSK_IS_WEBGPU_PIPELINE_MANAGER (self));
  
  if (cache_hits)
    *cache_hits = self->pipeline_cache_hits;
  if (cache_misses)  
    *cache_misses = self->pipeline_cache_misses;
}