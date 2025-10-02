/*
 * GTK Scaling Infrastructure WASM Module
 * Copyright (c) 2025 Superstruct Ltd, New Zealand
 *
 * Load balancing and scaling infrastructure for GTK4 WebGPU distributed 1M users.
 * Each user runs in their own browser - this coordinates resource distribution.
 */

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Resource distribution strategy
typedef struct {
    // CPU/Memory scaling
    uint32_t cpu_cores_allocated;
    size_t memory_limit_mb;
    bool adaptive_memory_scaling;
    double cpu_usage_threshold;

    // WebGPU resource distribution
    uint32_t gpu_memory_allocation_mb;
    uint32_t max_concurrent_render_passes;
    uint32_t texture_cache_size_mb;
    bool gpu_resource_pooling;

    // Widget rendering load balancing
    uint32_t widgets_per_frame_budget;
    bool hierarchical_culling;
    bool adaptive_lod;
    double frame_time_budget_ms;

    // Asset distribution
    bool cdn_load_balancing;
    bool progressive_asset_loading;
    bool intelligent_prefetching;
    uint32_t concurrent_downloads;

    // Network optimization
    bool http2_multiplexing;
    bool compression_enabled;
    bool connection_pooling;
    uint32_t max_connections;

    // User experience scaling
    bool quality_adaptation;
    bool bandwidth_detection;
    bool device_capability_detection;
    double min_fps_target;

    bool initialized;
} GtkScalingConfig;

// System metrics for scaling decisions
typedef struct {
    // Current load
    double cpu_usage_percent;
    size_t memory_usage_mb;
    double gpu_utilization_percent;
    uint32_t active_widgets;

    // Performance metrics
    double average_frame_time_ms;
    double p95_frame_time_ms;
    uint32_t dropped_frames;
    double network_latency_ms;

    // Resource utilization
    uint32_t allocated_gpu_memory_mb;
    uint32_t texture_cache_usage_mb;
    uint32_t active_render_passes;
    uint32_t pending_downloads;

    // Quality metrics
    double quality_score;
    double user_satisfaction_score;
    bool performance_target_met;

    double last_update_time_ms;
} GtkSystemMetrics;

// Scaling decision structure
typedef struct {
    // Resource adjustments
    bool increase_memory_limit;
    bool decrease_memory_limit;
    bool increase_gpu_allocation;
    bool decrease_gpu_allocation;

    // Quality adjustments
    bool increase_quality;
    bool decrease_quality;
    bool enable_lod;
    bool disable_lod;

    // Performance optimizations
    bool enable_culling;
    bool disable_culling;
    bool prioritize_visible_widgets;
    bool reduce_texture_quality;

    // Network optimizations
    bool increase_prefetch;
    bool decrease_prefetch;
    bool enable_compression;
    bool reduce_concurrent_downloads;

    double confidence_score;
    char reasoning[256];
} GtkScalingDecision;

// Global scaling state
static GtkScalingConfig scaling_config = {0};
static GtkSystemMetrics current_metrics = {0};
static GtkScalingDecision last_decision = {0};
static bool scaling_enabled = false;

// Scaling history for machine learning
#define SCALING_HISTORY_SIZE 1000
static GtkSystemMetrics metrics_history[SCALING_HISTORY_SIZE];
static uint32_t history_index = 0;
static bool history_full = false;

// Initialize scaling infrastructure
EMSCRIPTEN_KEEPALIVE
bool gtk_scaling_init(void) {
    scaling_enabled = true;

    // Set optimal defaults for distributed 1M users
    scaling_config.cpu_cores_allocated = 4;  // Assume quad-core minimum
    scaling_config.memory_limit_mb = 512;    // Per-user memory limit
    scaling_config.adaptive_memory_scaling = true;
    scaling_config.cpu_usage_threshold = 80.0;

    scaling_config.gpu_memory_allocation_mb = 128;  // Per-user GPU memory
    scaling_config.max_concurrent_render_passes = 4;
    scaling_config.texture_cache_size_mb = 64;
    scaling_config.gpu_resource_pooling = true;

    scaling_config.widgets_per_frame_budget = 1000;
    scaling_config.hierarchical_culling = true;
    scaling_config.adaptive_lod = true;
    scaling_config.frame_time_budget_ms = 16.67;  // 60 FPS

    scaling_config.cdn_load_balancing = true;
    scaling_config.progressive_asset_loading = true;
    scaling_config.intelligent_prefetching = true;
    scaling_config.concurrent_downloads = 6;

    scaling_config.http2_multiplexing = true;
    scaling_config.compression_enabled = true;
    scaling_config.connection_pooling = true;
    scaling_config.max_connections = 6;

    scaling_config.quality_adaptation = true;
    scaling_config.bandwidth_detection = true;
    scaling_config.device_capability_detection = true;
    scaling_config.min_fps_target = 55.0;

    scaling_config.initialized = true;

    // Initialize metrics
    memset(&current_metrics, 0, sizeof(current_metrics));
    memset(metrics_history, 0, sizeof(metrics_history));
    current_metrics.last_update_time_ms = emscripten_performance_now();

    printf("GTK Scaling Infrastructure initialized for 1M distributed users\n");
    return true;
}

// Update system metrics
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_update_metrics(double cpu_usage, size_t memory_mb, double gpu_usage,
                               uint32_t widgets, double frame_time_ms, double network_latency) {
    if (!scaling_enabled) return;

    // Store previous metrics in history
    metrics_history[history_index] = current_metrics;
    history_index = (history_index + 1) % SCALING_HISTORY_SIZE;
    if (history_index == 0) history_full = true;

    // Update current metrics
    current_metrics.cpu_usage_percent = cpu_usage;
    current_metrics.memory_usage_mb = memory_mb;
    current_metrics.gpu_utilization_percent = gpu_usage;
    current_metrics.active_widgets = widgets;
    current_metrics.network_latency_ms = network_latency;

    // Update frame time metrics with smoothing
    double alpha = 0.1;  // Smoothing factor
    current_metrics.average_frame_time_ms =
        alpha * frame_time_ms + (1.0 - alpha) * current_metrics.average_frame_time_ms;

    // Track p95 frame time (simplified)
    if (frame_time_ms > current_metrics.p95_frame_time_ms) {
        current_metrics.p95_frame_time_ms =
            0.05 * frame_time_ms + 0.95 * current_metrics.p95_frame_time_ms;
    }

    // Count dropped frames
    if (frame_time_ms > scaling_config.frame_time_budget_ms * 1.5) {
        current_metrics.dropped_frames++;
    }

    // Calculate performance target achievement
    current_metrics.performance_target_met =
        (current_metrics.average_frame_time_ms <= scaling_config.frame_time_budget_ms) &&
        (current_metrics.cpu_usage_percent <= scaling_config.cpu_usage_threshold) &&
        (current_metrics.memory_usage_mb <= scaling_config.memory_limit_mb);

    // Calculate quality score (0-100)
    double frame_score = 100.0 * (scaling_config.frame_time_budget_ms /
                                 fmax(current_metrics.average_frame_time_ms, scaling_config.frame_time_budget_ms));
    double resource_score = 100.0 * (1.0 - current_metrics.cpu_usage_percent / 100.0);
    current_metrics.quality_score = (frame_score + resource_score) / 2.0;

    current_metrics.last_update_time_ms = emscripten_performance_now();
}

// Make scaling decision based on current metrics
EMSCRIPTEN_KEEPALIVE
GtkScalingDecision* gtk_scaling_make_decision(void) {
    if (!scaling_enabled) return NULL;

    memset(&last_decision, 0, sizeof(last_decision));

    // Memory scaling decisions
    if (current_metrics.memory_usage_mb > scaling_config.memory_limit_mb * 0.9) {
        if (current_metrics.performance_target_met) {
            last_decision.reduce_texture_quality = true;
            last_decision.enable_culling = true;
            strcpy(last_decision.reasoning, "High memory usage - optimizing resource usage");
        } else {
            last_decision.decrease_memory_limit = true;
            strcpy(last_decision.reasoning, "Memory pressure - reducing allocation");
        }
    } else if (current_metrics.memory_usage_mb < scaling_config.memory_limit_mb * 0.5) {
        if (current_metrics.quality_score < 80.0) {
            last_decision.increase_memory_limit = true;
            last_decision.increase_quality = true;
            strcpy(last_decision.reasoning, "Low memory usage - increasing quality");
        }
    }

    // GPU scaling decisions
    if (current_metrics.gpu_utilization_percent > 85.0) {
        last_decision.enable_lod = true;
        last_decision.prioritize_visible_widgets = true;
        strcpy(last_decision.reasoning, "High GPU usage - enabling LOD and culling");
    } else if (current_metrics.gpu_utilization_percent < 50.0) {
        last_decision.disable_lod = true;
        last_decision.increase_quality = true;
        strcpy(last_decision.reasoning, "Low GPU usage - increasing quality");
    }

    // Frame rate optimization
    if (current_metrics.average_frame_time_ms > scaling_config.frame_time_budget_ms * 1.2) {
        last_decision.enable_culling = true;
        last_decision.enable_lod = true;
        last_decision.decrease_quality = true;
        strcpy(last_decision.reasoning, "Frame rate below target - aggressive optimization");
    }

    // Network optimization
    if (current_metrics.network_latency_ms > 100.0) {
        last_decision.enable_compression = true;
        last_decision.reduce_concurrent_downloads = true;
        strcpy(last_decision.reasoning, "High network latency - optimizing transfers");
    } else if (current_metrics.network_latency_ms < 20.0) {
        last_decision.increase_prefetch = true;
        strcpy(last_decision.reasoning, "Low network latency - increasing prefetch");
    }

    // Calculate confidence based on metric stability
    double confidence = 0.8;  // Base confidence
    if (history_full) {
        // Check metric stability over time
        double variance = 0.0;
        uint32_t samples = history_full ? SCALING_HISTORY_SIZE : history_index;
        for (uint32_t i = 0; i < samples; i++) {
            double diff = metrics_history[i].average_frame_time_ms - current_metrics.average_frame_time_ms;
            variance += diff * diff;
        }
        variance /= samples;

        // Higher variance = lower confidence
        confidence *= exp(-variance / 100.0);
    }

    last_decision.confidence_score = confidence;

    return &last_decision;
}

// Apply scaling decision
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_apply_decision(const GtkScalingDecision* decision) {
    if (!scaling_enabled || !decision) return;

    printf("Applying scaling decision: %s (confidence: %.2f)\n",
           decision->reasoning, decision->confidence_score);

    // Apply memory adjustments
    if (decision->increase_memory_limit) {
        scaling_config.memory_limit_mb = (uint32_t)(scaling_config.memory_limit_mb * 1.2);
        printf("Increased memory limit to %u MB\n", scaling_config.memory_limit_mb);
    }
    if (decision->decrease_memory_limit) {
        scaling_config.memory_limit_mb = (uint32_t)(scaling_config.memory_limit_mb * 0.8);
        printf("Decreased memory limit to %u MB\n", scaling_config.memory_limit_mb);
    }

    // Apply GPU adjustments
    if (decision->increase_gpu_allocation) {
        scaling_config.gpu_memory_allocation_mb = (uint32_t)(scaling_config.gpu_memory_allocation_mb * 1.2);
        printf("Increased GPU allocation to %u MB\n", scaling_config.gpu_memory_allocation_mb);
    }
    if (decision->decrease_gpu_allocation) {
        scaling_config.gpu_memory_allocation_mb = (uint32_t)(scaling_config.gpu_memory_allocation_mb * 0.8);
        printf("Decreased GPU allocation to %u MB\n", scaling_config.gpu_memory_allocation_mb);
    }

    // Apply quality adjustments
    if (decision->enable_lod) {
        scaling_config.adaptive_lod = true;
        printf("Enabled adaptive LOD\n");
    }
    if (decision->disable_lod) {
        scaling_config.adaptive_lod = false;
        printf("Disabled adaptive LOD\n");
    }

    // Apply performance optimizations
    if (decision->enable_culling) {
        scaling_config.hierarchical_culling = true;
        printf("Enabled hierarchical culling\n");
    }
    if (decision->prioritize_visible_widgets) {
        scaling_config.widgets_per_frame_budget = (uint32_t)(scaling_config.widgets_per_frame_budget * 0.8);
        printf("Reduced widget budget to %u per frame\n", scaling_config.widgets_per_frame_budget);
    }

    // Apply network optimizations
    if (decision->reduce_concurrent_downloads) {
        scaling_config.concurrent_downloads = (uint32_t)fmax(2, scaling_config.concurrent_downloads - 1);
        printf("Reduced concurrent downloads to %u\n", scaling_config.concurrent_downloads);
    }
    if (decision->increase_prefetch) {
        scaling_config.intelligent_prefetching = true;
        printf("Enabled intelligent prefetching\n");
    }
}

// Auto-scaling loop (called periodically)
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_auto_adjust(void) {
    if (!scaling_enabled) return;

    GtkScalingDecision* decision = gtk_scaling_make_decision();
    if (decision && decision->confidence_score > 0.7) {
        gtk_scaling_apply_decision(decision);
    }
}

// Get current scaling configuration
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_get_config(char* buffer, size_t buffer_size) {
    if (!scaling_enabled || !buffer) return;

    snprintf(buffer, buffer_size,
        "GTK Scaling Configuration (1M Users)\n"
        "====================================\n"
        "Memory Limit: %u MB\n"
        "GPU Allocation: %u MB\n"
        "Widgets per Frame: %u\n"
        "Frame Time Budget: %.2f ms\n"
        "Concurrent Downloads: %u\n"
        "Adaptive LOD: %s\n"
        "Hierarchical Culling: %s\n"
        "Quality Adaptation: %s\n"
        "CDN Load Balancing: %s\n"
        "\nCurrent Performance:\n"
        "CPU Usage: %.1f%%\n"
        "Memory Usage: %zu MB\n"
        "GPU Usage: %.1f%%\n"
        "Average Frame Time: %.2f ms\n"
        "Quality Score: %.1f/100\n"
        "Performance Target: %s\n",
        scaling_config.memory_limit_mb,
        scaling_config.gpu_memory_allocation_mb,
        scaling_config.widgets_per_frame_budget,
        scaling_config.frame_time_budget_ms,
        scaling_config.concurrent_downloads,
        scaling_config.adaptive_lod ? "Enabled" : "Disabled",
        scaling_config.hierarchical_culling ? "Enabled" : "Disabled",
        scaling_config.quality_adaptation ? "Enabled" : "Disabled",
        scaling_config.cdn_load_balancing ? "Enabled" : "Disabled",
        current_metrics.cpu_usage_percent,
        current_metrics.memory_usage_mb,
        current_metrics.gpu_utilization_percent,
        current_metrics.average_frame_time_ms,
        current_metrics.quality_score,
        current_metrics.performance_target_met ? "MET" : "NOT MET"
    );
}

// Export metrics as JSON for monitoring systems
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_export_metrics(char* buffer, size_t buffer_size) {
    if (!scaling_enabled || !buffer) return;

    snprintf(buffer, buffer_size,
        "{\n"
        "  \"scaling_config\": {\n"
        "    \"memory_limit_mb\": %u,\n"
        "    \"gpu_allocation_mb\": %u,\n"
        "    \"widgets_per_frame\": %u,\n"
        "    \"frame_budget_ms\": %.2f,\n"
        "    \"adaptive_lod\": %s,\n"
        "    \"hierarchical_culling\": %s\n"
        "  },\n"
        "  \"current_metrics\": {\n"
        "    \"cpu_usage\": %.2f,\n"
        "    \"memory_usage_mb\": %zu,\n"
        "    \"gpu_utilization\": %.2f,\n"
        "    \"active_widgets\": %u,\n"
        "    \"frame_time_ms\": %.2f,\n"
        "    \"quality_score\": %.2f,\n"
        "    \"performance_target_met\": %s\n"
        "  },\n"
        "  \"last_decision\": {\n"
        "    \"reasoning\": \"%s\",\n"
        "    \"confidence\": %.3f\n"
        "  }\n"
        "}",
        scaling_config.memory_limit_mb,
        scaling_config.gpu_memory_allocation_mb,
        scaling_config.widgets_per_frame_budget,
        scaling_config.frame_time_budget_ms,
        scaling_config.adaptive_lod ? "true" : "false",
        scaling_config.hierarchical_culling ? "true" : "false",
        current_metrics.cpu_usage_percent,
        current_metrics.memory_usage_mb,
        current_metrics.gpu_utilization_percent,
        current_metrics.active_widgets,
        current_metrics.average_frame_time_ms,
        current_metrics.quality_score,
        current_metrics.performance_target_met ? "true" : "false",
        last_decision.reasoning,
        last_decision.confidence_score
    );
}

// Check if system is optimally scaled
EMSCRIPTEN_KEEPALIVE
bool gtk_scaling_is_optimal(void) {
    if (!scaling_enabled) return false;

    return current_metrics.performance_target_met &&
           current_metrics.quality_score > 85.0 &&
           current_metrics.cpu_usage_percent < 80.0 &&
           current_metrics.gpu_utilization_percent < 80.0;
}

// Reset scaling configuration to defaults
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_reset_config(void) {
    if (!scaling_enabled) return;

    // Reset to optimal defaults
    gtk_scaling_init();
    printf("Scaling configuration reset to defaults\n");
}

// Shutdown scaling infrastructure
EMSCRIPTEN_KEEPALIVE
void gtk_scaling_shutdown(void) {
    scaling_enabled = false;
    printf("GTK Scaling Infrastructure shut down\n");
}