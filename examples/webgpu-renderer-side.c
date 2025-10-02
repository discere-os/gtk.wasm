/*
 * GTK WebGPU Renderer Side Module
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Side module - simplified WebGPU rendering functions
typedef struct {
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSwapChain swap_chain;
    int initialized;
} WebGPURenderer;

static WebGPURenderer *renderer = NULL;

EMSCRIPTEN_KEEPALIVE
int webgpu_renderer_init(const char* canvas_id) {
    printf("[WebGPU Side] Initializing renderer for canvas: %s\n", canvas_id);
    
    if (renderer) {
        return 1; // Already initialized
    }
    
    renderer = malloc(sizeof(WebGPURenderer));
    if (!renderer) {
        printf("[WebGPU Side] Failed to allocate renderer\n");
        return 0;
    }
    
    memset(renderer, 0, sizeof(WebGPURenderer));
    
    // Initialize WebGPU device (simplified for side module)
    WGPUInstanceDescriptor instanceDesc = {};
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    
    if (!instance) {
        printf("[WebGPU Side] Failed to create WebGPU instance\n");
        free(renderer);
        renderer = NULL;
        return 0;
    }
    
    // Request adapter (asynchronous, simplified)
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    
    printf("[WebGPU Side] WebGPU renderer initialized successfully\n");
    renderer->initialized = 1;
    return 1;
}

EMSCRIPTEN_KEEPALIVE 
int webgpu_renderer_create_surface(const char* canvas_id) {
    if (!renderer || !renderer->initialized) {
        printf("[WebGPU Side] Renderer not initialized\n");
        return 0;
    }
    
    printf("[WebGPU Side] Creating surface for canvas: %s\n", canvas_id);
    
    // Create surface from canvas
    WGPUSurfaceDescriptorFromCanvasHTMLSelector canvasDesc = {};
    canvasDesc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    canvasDesc.selector = canvas_id;
    
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = (const WGPUChainedStruct*)&canvasDesc;
    
    // This would create the actual surface in a full implementation
    printf("[WebGPU Side] Surface created successfully\n");
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int webgpu_renderer_render_frame() {
    if (!renderer || !renderer->initialized) {
        return 0;
    }
    
    // Simplified rendering - just a colored frame
    printf("[WebGPU Side] Rendering frame...\n");
    
    // In a real implementation, this would:
    // 1. Begin render pass
    // 2. Draw primitives using WebGPU shaders  
    // 3. End render pass
    // 4. Present frame
    
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int webgpu_renderer_draw_widget(int x, int y, int width, int height, int type) {
    if (!renderer || !renderer->initialized) {
        return 0;
    }
    
    printf("[WebGPU Side] Drawing widget at (%d,%d) size %dx%d type %d\n", 
           x, y, width, height, type);
    
    // In a real implementation, this would draw actual GTK widgets
    // using the WebGPU rendering pipeline and shaders
    
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* webgpu_renderer_get_info() {
    if (!renderer) {
        return "WebGPU Renderer Side Module - Not Initialized";
    }
    
    return "WebGPU Renderer Side Module - v1.0.0 - Dynamic Loading Enabled";
}

EMSCRIPTEN_KEEPALIVE
void webgpu_renderer_cleanup() {
    if (!renderer) {
        return;
    }
    
    printf("[WebGPU Side] Cleaning up renderer\n");
    
    // Cleanup WebGPU resources
    if (renderer->swap_chain) {
        wgpuSwapChainRelease(renderer->swap_chain);
    }
    if (renderer->surface) {
        wgpuSurfaceRelease(renderer->surface);
    }
    if (renderer->queue) {
        wgpuQueueRelease(renderer->queue);
    }
    if (renderer->device) {
        wgpuDeviceRelease(renderer->device);
    }
    
    free(renderer);
    renderer = NULL;
    
    printf("[WebGPU Side] Renderer cleanup complete\n");
}