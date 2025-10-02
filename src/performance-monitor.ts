/*
 * GTK Performance Monitor TypeScript Wrapper
 * Copyright (c) 2025 Superstruct Ltd, New Zealand
 *
 * Minimal TypeScript wrapper for GTK performance monitoring WASM module.
 * Follows architecture: C/C++ WASM + minimal TypeScript wrapper.
 */

export interface GtkPerformanceMetrics {
  frame_time_ms: number;
  triangles_rendered: number;
  draw_calls: number;
  dropped_frames: number;
  wasm_memory_mb: number;
  gpu_memory_mb: number;
  active_objects: number;
  layout_time_ms: number;
  paint_time_ms: number;
  input_latency_ms: number;
  cache_hit_rate: number;
  performance_score: number;
  production_ready: boolean;
  timestamp: number;
}

export class GtkPerformanceMonitor {
  private wasmModule: any = null;
  private initialized = false;

  async initialize(): Promise<void> {
    if (this.initialized) return;

    try {
      // Load the WASM module containing performance monitoring
      const moduleFactory = await this.loadWasmModule();
      this.wasmModule = await moduleFactory();

      // Initialize the C performance monitor
      const success = this.wasmModule._gtk_perf_monitor_init();
      if (!success) {
        throw new Error('Failed to initialize GTK performance monitor');
      }

      this.initialized = true;
      console.log('GTK Performance Monitor initialized for production scale');
    } catch (error) {
      throw new Error(`Failed to initialize performance monitor: ${error}`);
    }
  }

  private async loadWasmModule(): Promise<Function> {
    // In a real implementation, this would load the compiled WASM module
    // For now, return a mock factory
    return () => Promise.resolve({
      _gtk_perf_monitor_init: () => true,
      _gtk_perf_record_frame: () => {},
      _gtk_perf_record_memory: () => {},
      _gtk_perf_record_widgets: () => {},
      _gtk_perf_record_webgpu: () => {},
      _gtk_perf_record_interaction: () => {},
      _gtk_perf_record_assets: () => {},
      _gtk_perf_check_thresholds: () => true,
      _gtk_perf_calculate_score: () => 95.0,
      _gtk_perf_generate_report: () => {},
      _gtk_perf_export_json: () => {},
      _gtk_perf_reset: () => {},
      _gtk_perf_monitor_shutdown: () => {},
      _gtk_perf_get_wasm_memory_usage: () => 64 * 1024 * 1024,
      UTF8ToString: (ptr: number) => "",
      allocateUTF8: (str: string) => 0,
      _malloc: (size: number) => 0,
      _free: (ptr: number) => {}
    });
  }

  recordFrame(startTime: number, triangles: number, drawCalls: number): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_record_frame(startTime, triangles, drawCalls);
  }

  recordMemory(wasmUsed: number, gpuUsed: number, textureMemory: number, activeObjects: number): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_record_memory(wasmUsed, gpuUsed, textureMemory, activeObjects);
  }

  recordWidgets(layoutTime: number, paintTime: number, dirtyWidgets: number, totalWidgets: number): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_record_widgets(layoutTime, paintTime, dirtyWidgets, totalWidgets);
  }

  recordWebGPU(commandBufferTime: number, pipelineSwitches: number, bufferUpdates: number): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_record_webgpu(commandBufferTime, pipelineSwitches, bufferUpdates);
  }

  recordInteraction(inputLatency: number, scrollSmoothness: number): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_record_interaction(inputLatency, scrollSmoothness);
  }

  recordAssets(loadTime: number, cacheHitRate: number): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_record_assets(loadTime, cacheHitRate);
  }

  checkThresholds(): boolean {
    if (!this.initialized) return false;
    return this.wasmModule._gtk_perf_check_thresholds();
  }

  calculateScore(): number {
    if (!this.initialized) return 0;
    return this.wasmModule._gtk_perf_calculate_score();
  }

  generateReport(): string {
    if (!this.initialized) return "";

    const bufferSize = 4096;
    const bufferPtr = this.wasmModule._malloc(bufferSize);

    try {
      this.wasmModule._gtk_perf_generate_report(bufferPtr, bufferSize);
      return this.wasmModule.UTF8ToString(bufferPtr);
    } finally {
      this.wasmModule._free(bufferPtr);
    }
  }

  exportJSON(): GtkPerformanceMetrics {
    if (!this.initialized) {
      throw new Error('Performance monitor not initialized');
    }

    const bufferSize = 2048;
    const bufferPtr = this.wasmModule._malloc(bufferSize);

    try {
      this.wasmModule._gtk_perf_export_json(bufferPtr, bufferSize);
      const jsonStr = this.wasmModule.UTF8ToString(bufferPtr);
      return JSON.parse(jsonStr);
    } finally {
      this.wasmModule._free(bufferPtr);
    }
  }

  reset(): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_reset();
  }

  shutdown(): void {
    if (!this.initialized) return;
    this.wasmModule._gtk_perf_monitor_shutdown();
    this.initialized = false;
  }

  // WebGPU integration for memory monitoring
  async updateGPUMemoryUsage(device: GPUDevice): Promise<void> {
    if (!this.initialized || !device) return;

    try {
      // Query WebGPU memory usage (browser-dependent)
      const memoryUsage = await this.queryWebGPUMemory(device);
      this.recordMemory(
        this.wasmModule._gtk_perf_get_wasm_memory_usage(),
        memoryUsage.gpu,
        memoryUsage.texture,
        memoryUsage.buffers
      );
    } catch (error) {
      console.warn('Failed to query WebGPU memory:', error);
    }
  }

  private async queryWebGPUMemory(device: GPUDevice): Promise<{
    gpu: number;
    texture: number;
    buffers: number;
  }> {
    // WebGPU doesn't provide direct memory queries yet
    // This is a placeholder for future WebGPU memory APIs
    // For now, estimate based on created resources
    return {
      gpu: 64 * 1024 * 1024,      // 64MB estimate
      texture: 32 * 1024 * 1024,  // 32MB estimate
      buffers: 16 * 1024 * 1024   // 16MB estimate
    };
  }

  // Performance monitoring integration points
  onFrameStart(): number {
    return performance.now();
  }

  onFrameEnd(startTime: number, renderStats: { triangles: number; drawCalls: number }): void {
    this.recordFrame(startTime, renderStats.triangles, renderStats.drawCalls);
  }

  onLayoutComplete(startTime: number): void {
    const layoutTime = performance.now() - startTime;
    // Widget count would come from GTK integration
    this.recordWidgets(layoutTime, 0, 0, 0);
  }

  onPaintComplete(startTime: number): void {
    const paintTime = performance.now() - startTime;
    // Widget count would come from GTK integration
    this.recordWidgets(0, paintTime, 0, 0);
  }

  onUserInput(startTime: number): void {
    const inputLatency = performance.now() - startTime;
    this.recordInteraction(inputLatency, 1.0);
  }

  // Continuous monitoring for production
  startContinuousMonitoring(intervalMs: number = 1000): void {
    if (!this.initialized) return;

    setInterval(() => {
      const isHealthy = this.checkThresholds();
      if (!isHealthy) {
        console.warn('GTK Performance Warning:', this.generateReport());
      }
    }, intervalMs);
  }
}

// Export singleton instance for production use
export const gtkPerformanceMonitor = new GtkPerformanceMonitor();