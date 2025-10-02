/*
 * GTK Performance Monitoring WASM Module
 * Copyright (c) 2025 Superstruct Ltd, New Zealand
 *
 * Production-grade performance monitoring for GTK4 WebGPU at 1M user scale.
 * Each user runs this in their own browser memory space.
 */

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Performance metrics structure
typedef struct {
    // Rendering performance
    double frame_time_ms;
    double gpu_utilization;
    uint32_t triangles_rendered;
    uint32_t draw_calls;

    // Memory usage
    size_t wasm_memory_used;
    size_t gpu_memory_used;
    size_t texture_memory;
    uint32_t active_objects;

    // Widget performance
    double layout_time_ms;
    double paint_time_ms;
    uint32_t dirty_widgets;
    uint32_t total_widgets;

    // WebGPU specific
    double command_buffer_time_ms;
    uint32_t pipeline_switches;
    uint32_t buffer_updates;

    // User interaction
    double input_latency_ms;
    double scroll_smoothness;
    uint32_t dropped_frames;

    // Network/CDN
    double wasm_load_time_ms;
    double asset_cache_hit_rate;

    // Timestamp
    double timestamp_ms;
} GtkPerformanceMetrics;

// Global metrics state
static GtkPerformanceMetrics current_metrics = {0};
static GtkPerformanceMetrics baseline_metrics = {0};
static bool monitoring_enabled = false;
static double last_frame_time = 0;

// Performance thresholds for 1M user scale
#define TARGET_FRAME_TIME_MS 16.67     // 60 FPS
#define MAX_MEMORY_MB 512              // Per-user memory limit
#define MAX_GPU_MEMORY_MB 128          // Per-user GPU memory
#define MAX_INPUT_LATENCY_MS 20        // Responsive interaction
#define MIN_CACHE_HIT_RATE 0.85        // 85% cache efficiency

// Initialize performance monitoring
EMSCRIPTEN_KEEPALIVE
bool gtk_perf_monitor_init(void) {
    monitoring_enabled = true;
    memset(&current_metrics, 0, sizeof(current_metrics));
    memset(&baseline_metrics, 0, sizeof(baseline_metrics));

    // Record baseline timestamp
    current_metrics.timestamp_ms = emscripten_performance_now();
    baseline_metrics = current_metrics;

    printf("GTK Performance Monitor initialized for production scale\n");
    return true;
}

// Record frame rendering metrics
EMSCRIPTEN_KEEPALIVE
void gtk_perf_record_frame(double frame_start_ms, uint32_t triangles, uint32_t draws) {
    if (!monitoring_enabled) return;

    double frame_end_ms = emscripten_performance_now();
    current_metrics.frame_time_ms = frame_end_ms - frame_start_ms;
    current_metrics.triangles_rendered = triangles;
    current_metrics.draw_calls = draws;
    current_metrics.timestamp_ms = frame_end_ms;

    // Track dropped frames
    if (current_metrics.frame_time_ms > TARGET_FRAME_TIME_MS * 1.5) {
        current_metrics.dropped_frames++;
    }

    last_frame_time = frame_end_ms;
}

// Record memory usage metrics
EMSCRIPTEN_KEEPALIVE
void gtk_perf_record_memory(size_t wasm_used, size_t gpu_used, size_t textures, uint32_t objects) {
    if (!monitoring_enabled) return;

    current_metrics.wasm_memory_used = wasm_used;
    current_metrics.gpu_memory_used = gpu_used;
    current_metrics.texture_memory = textures;
    current_metrics.active_objects = objects;
}

// Record widget layout and paint performance
EMSCRIPTEN_KEEPALIVE
void gtk_perf_record_widgets(double layout_ms, double paint_ms, uint32_t dirty, uint32_t total) {
    if (!monitoring_enabled) return;

    current_metrics.layout_time_ms = layout_ms;
    current_metrics.paint_time_ms = paint_ms;
    current_metrics.dirty_widgets = dirty;
    current_metrics.total_widgets = total;
}

// Record WebGPU command submission performance
EMSCRIPTEN_KEEPALIVE
void gtk_perf_record_webgpu(double cmd_buffer_ms, uint32_t pipeline_switches, uint32_t buffer_updates) {
    if (!monitoring_enabled) return;

    current_metrics.command_buffer_time_ms = cmd_buffer_ms;
    current_metrics.pipeline_switches = pipeline_switches;
    current_metrics.buffer_updates = buffer_updates;
}

// Record user interaction responsiveness
EMSCRIPTEN_KEEPALIVE
void gtk_perf_record_interaction(double input_latency, double scroll_smooth) {
    if (!monitoring_enabled) return;

    current_metrics.input_latency_ms = input_latency;
    current_metrics.scroll_smoothness = scroll_smooth;
}

// Record asset loading and caching performance
EMSCRIPTEN_KEEPALIVE
void gtk_perf_record_assets(double load_time, double cache_hit_rate) {
    if (!monitoring_enabled) return;

    current_metrics.wasm_load_time_ms = load_time;
    current_metrics.asset_cache_hit_rate = cache_hit_rate;
}

// Get current performance metrics
EMSCRIPTEN_KEEPALIVE
GtkPerformanceMetrics* gtk_perf_get_metrics(void) {
    if (!monitoring_enabled) return NULL;
    return &current_metrics;
}

// Check if performance meets production thresholds
EMSCRIPTEN_KEEPALIVE
bool gtk_perf_check_thresholds(void) {
    if (!monitoring_enabled) return false;

    bool meets_targets = true;

    // Frame rate check
    if (current_metrics.frame_time_ms > TARGET_FRAME_TIME_MS * 1.2) {
        printf("WARNING: Frame time %.2fms exceeds target %.2fms\n",
               current_metrics.frame_time_ms, TARGET_FRAME_TIME_MS);
        meets_targets = false;
    }

    // Memory usage check
    size_t wasm_mb = current_metrics.wasm_memory_used / (1024 * 1024);
    if (wasm_mb > MAX_MEMORY_MB) {
        printf("WARNING: WASM memory %zuMB exceeds limit %dMB\n", wasm_mb, MAX_MEMORY_MB);
        meets_targets = false;
    }

    size_t gpu_mb = current_metrics.gpu_memory_used / (1024 * 1024);
    if (gpu_mb > MAX_GPU_MEMORY_MB) {
        printf("WARNING: GPU memory %zuMB exceeds limit %dMB\n", gpu_mb, MAX_GPU_MEMORY_MB);
        meets_targets = false;
    }

    // Input responsiveness check
    if (current_metrics.input_latency_ms > MAX_INPUT_LATENCY_MS) {
        printf("WARNING: Input latency %.2fms exceeds target %dms\n",
               current_metrics.input_latency_ms, MAX_INPUT_LATENCY_MS);
        meets_targets = false;
    }

    // Cache efficiency check
    if (current_metrics.asset_cache_hit_rate < MIN_CACHE_HIT_RATE) {
        printf("WARNING: Cache hit rate %.2f%% below target %.0f%%\n",
               current_metrics.asset_cache_hit_rate * 100, MIN_CACHE_HIT_RATE * 100);
        meets_targets = false;
    }

    return meets_targets;
}

// Calculate performance score (0-100)
EMSCRIPTEN_KEEPALIVE
double gtk_perf_calculate_score(void) {
    if (!monitoring_enabled) return 0.0;

    double score = 100.0;

    // Frame rate score (40% weight)
    double frame_score = 100.0 * (TARGET_FRAME_TIME_MS / fmax(current_metrics.frame_time_ms, TARGET_FRAME_TIME_MS));
    score = score * 0.6 + frame_score * 0.4;

    // Memory efficiency score (20% weight)
    double wasm_usage = (double)current_metrics.wasm_memory_used / (MAX_MEMORY_MB * 1024 * 1024);
    double gpu_usage = (double)current_metrics.gpu_memory_used / (MAX_GPU_MEMORY_MB * 1024 * 1024);
    double memory_score = 100.0 * (1.0 - fmax(wasm_usage, gpu_usage));
    score = score * 0.8 + memory_score * 0.2;

    // Input responsiveness score (20% weight)
    double input_score = 100.0 * (MAX_INPUT_LATENCY_MS / fmax(current_metrics.input_latency_ms, MAX_INPUT_LATENCY_MS));
    score = score * 0.8 + input_score * 0.2;

    // Cache efficiency score (20% weight)
    double cache_score = current_metrics.asset_cache_hit_rate * 100.0;
    score = score * 0.8 + cache_score * 0.2;

    return fmax(0.0, fmin(100.0, score));
}

// Generate production performance report
EMSCRIPTEN_KEEPALIVE
void gtk_perf_generate_report(char* buffer, size_t buffer_size) {
    if (!monitoring_enabled || !buffer) return;

    double score = gtk_perf_calculate_score();
    bool meets_thresholds = gtk_perf_check_thresholds();

    snprintf(buffer, buffer_size,
        "GTK Performance Report (Production Scale)\n"
        "========================================\n"
        "Overall Score: %.1f/100 %s\n"
        "Timestamp: %.3f ms\n\n"
        "Rendering Performance:\n"
        "  Frame Time: %.2f ms (target: %.2f ms)\n"
        "  Triangles: %u\n"
        "  Draw Calls: %u\n"
        "  Dropped Frames: %u\n\n"
        "Memory Usage:\n"
        "  WASM Memory: %.1f MB (limit: %d MB)\n"
        "  GPU Memory: %.1f MB (limit: %d MB)\n"
        "  Texture Memory: %.1f MB\n"
        "  Active Objects: %u\n\n"
        "Widget Performance:\n"
        "  Layout Time: %.2f ms\n"
        "  Paint Time: %.2f ms\n"
        "  Dirty Widgets: %u / %u\n\n"
        "WebGPU Performance:\n"
        "  Command Buffer: %.2f ms\n"
        "  Pipeline Switches: %u\n"
        "  Buffer Updates: %u\n\n"
        "User Experience:\n"
        "  Input Latency: %.2f ms (target: %d ms)\n"
        "  Scroll Smoothness: %.2f\n\n"
        "Asset Performance:\n"
        "  WASM Load Time: %.2f ms\n"
        "  Cache Hit Rate: %.1f%% (target: %.0f%%)\n\n"
        "Production Ready: %s\n",
        score,
        meets_thresholds ? "✓" : "✗",
        current_metrics.timestamp_ms,
        current_metrics.frame_time_ms, TARGET_FRAME_TIME_MS,
        current_metrics.triangles_rendered,
        current_metrics.draw_calls,
        current_metrics.dropped_frames,
        current_metrics.wasm_memory_used / (1024.0 * 1024.0), MAX_MEMORY_MB,
        current_metrics.gpu_memory_used / (1024.0 * 1024.0), MAX_GPU_MEMORY_MB,
        current_metrics.texture_memory / (1024.0 * 1024.0),
        current_metrics.active_objects,
        current_metrics.layout_time_ms,
        current_metrics.paint_time_ms,
        current_metrics.dirty_widgets, current_metrics.total_widgets,
        current_metrics.command_buffer_time_ms,
        current_metrics.pipeline_switches,
        current_metrics.buffer_updates,
        current_metrics.input_latency_ms, MAX_INPUT_LATENCY_MS,
        current_metrics.scroll_smoothness,
        current_metrics.wasm_load_time_ms,
        current_metrics.asset_cache_hit_rate * 100.0, MIN_CACHE_HIT_RATE * 100.0,
        meets_thresholds ? "YES" : "NO"
    );
}

// Reset performance counters
EMSCRIPTEN_KEEPALIVE
void gtk_perf_reset(void) {
    if (!monitoring_enabled) return;

    memset(&current_metrics, 0, sizeof(current_metrics));
    current_metrics.timestamp_ms = emscripten_performance_now();

    printf("Performance metrics reset\n");
}

// Disable monitoring
EMSCRIPTEN_KEEPALIVE
void gtk_perf_monitor_shutdown(void) {
    monitoring_enabled = false;
    printf("GTK Performance Monitor shut down\n");
}

// Export metrics as JSON for external monitoring systems
EMSCRIPTEN_KEEPALIVE
void gtk_perf_export_json(char* buffer, size_t buffer_size) {
    if (!monitoring_enabled || !buffer) return;

    snprintf(buffer, buffer_size,
        "{\n"
        "  \"timestamp\": %.3f,\n"
        "  \"frame_time_ms\": %.2f,\n"
        "  \"triangles_rendered\": %u,\n"
        "  \"draw_calls\": %u,\n"
        "  \"dropped_frames\": %u,\n"
        "  \"wasm_memory_mb\": %.1f,\n"
        "  \"gpu_memory_mb\": %.1f,\n"
        "  \"active_objects\": %u,\n"
        "  \"layout_time_ms\": %.2f,\n"
        "  \"paint_time_ms\": %.2f,\n"
        "  \"input_latency_ms\": %.2f,\n"
        "  \"cache_hit_rate\": %.3f,\n"
        "  \"performance_score\": %.1f,\n"
        "  \"production_ready\": %s\n"
        "}",
        current_metrics.timestamp_ms,
        current_metrics.frame_time_ms,
        current_metrics.triangles_rendered,
        current_metrics.draw_calls,
        current_metrics.dropped_frames,
        current_metrics.wasm_memory_used / (1024.0 * 1024.0),
        current_metrics.gpu_memory_used / (1024.0 * 1024.0),
        current_metrics.active_objects,
        current_metrics.layout_time_ms,
        current_metrics.paint_time_ms,
        current_metrics.input_latency_ms,
        current_metrics.asset_cache_hit_rate,
        gtk_perf_calculate_score(),
        gtk_perf_check_thresholds() ? "true" : "false"
    );
}

// Get memory usage directly from WASM runtime
EMSCRIPTEN_KEEPALIVE
size_t gtk_perf_get_wasm_memory_usage(void) {
    // Emscripten provides access to current memory usage
    size_t pages = emscripten_get_heap_size() / 65536;  // WASM page size
    return pages * 65536;
}

// WebGPU memory query (requires WebGPU context)
EMSCRIPTEN_KEEPALIVE
void gtk_perf_query_gpu_memory(void) {
    // This will be called from TypeScript wrapper with WebGPU context
    // JavaScript/TypeScript side will call gtk_perf_record_memory() with results
}