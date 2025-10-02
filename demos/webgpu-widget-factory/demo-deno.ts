#!/usr/bin/env -S deno run --allow-read --allow-write --allow-net

// WebGPU GTK4 Widget Factory Demo - Deno runner
// MAIN module example hosting SIDE modules for full-stack testing

import WebGPUWidgetFactoryDemoImpl from './demo-webgpu.ts';

interface DemoConfig {
  canvasWidth: number;
  canvasHeight: number;
  enableStressTest: boolean;
  stressTestWidgets: number;
  enablePerformanceMonitoring: boolean;
  targetFPS: number;
}

const defaultConfig: DemoConfig = {
  canvasWidth: 1024,
  canvasHeight: 768,
  enableStressTest: false,
  stressTestWidgets: 1000,
  enablePerformanceMonitoring: true,
  targetFPS: 60
};

class DemoRunner {
  private demo: WebGPUWidgetFactoryDemoImpl;
  private canvas: HTMLCanvasElement | null = null;
  private animationId: number = 0;
  private config: DemoConfig;
  private performanceInterval: number = 0;

  constructor(config: Partial<DemoConfig> = {}) {
    this.demo = new WebGPUWidgetFactoryDemoImpl();
    this.config = { ...defaultConfig, ...config };
  }

  async start(): Promise<void> {
    console.log('🚀 Starting WebGPU GTK4 Widget Factory Demo...');

    try {
      // Check for Deno environment
      if (typeof globalThis.Deno === 'undefined') {
        throw new Error('This demo requires Deno runtime for development');
      }

      // Create virtual canvas for Deno environment
      this.canvas = this.createVirtualCanvas();

      console.log('📊 Initializing WebGPU renderer...');
      await this.demo.initialize(this.canvas);

      console.log('🎨 Starting render loop...');
      await this.startRenderLoop();

      if (this.config.enablePerformanceMonitoring) {
        console.log('📈 Starting performance monitoring...');
        this.startPerformanceMonitoring();
      }

      if (this.config.enableStressTest) {
        console.log(`🔥 Starting stress test with ${this.config.stressTestWidgets} widgets...`);
        await this.demo.startStressTest(this.config.stressTestWidgets);
      }

      console.log('✅ Demo started successfully!');
      console.log('   - Canvas size:', `${this.config.canvasWidth}x${this.config.canvasHeight}`);
      console.log('   - Target FPS:', this.config.targetFPS);
      console.log('   - Stress test:', this.config.enableStressTest ? 'enabled' : 'disabled');

      // Keep demo running
      await this.runForDuration(30000); // 30 seconds

    } catch (error) {
      console.error('❌ Demo failed to start:', error);
      throw error;
    }
  }

  private createVirtualCanvas(): HTMLCanvasElement {
    // Create a virtual canvas for Deno environment
    // This simulates the DOM canvas API for WASM module compatibility
    const canvas = {
      width: this.config.canvasWidth,
      height: this.config.canvasHeight,

      getContext: (contextType: string) => {
        if (contextType === 'webgpu') {
          return this.createVirtualWebGPUContext();
        }
        return null;
      },

      addEventListener: () => {},
      removeEventListener: () => {},
      dispatchEvent: () => true,

      // Simulate canvas element properties
      offsetWidth: this.config.canvasWidth,
      offsetHeight: this.config.canvasHeight,
      clientWidth: this.config.canvasWidth,
      clientHeight: this.config.canvasHeight,

      style: {},
      className: '',
      id: 'webgpu-demo-canvas'
    };

    return canvas as any as HTMLCanvasElement;
  }

  private createVirtualWebGPUContext(): any {
    // Virtual WebGPU context for Deno environment
    return {
      configure: (config: any) => {
        console.log('🎯 WebGPU context configured:', {
          format: config.format,
          alphaMode: config.alphaMode
        });
      },

      getCurrentTexture: () => ({
        createView: () => ({
          // Virtual texture view
        })
      }),

      presentationFormat: 'bgra8unorm'
    };
  }

  private async startRenderLoop(): Promise<void> {
    const targetFrameTime = 1000 / this.config.targetFPS;
    let lastTime = performance.now();

    const renderFrame = async () => {
      const currentTime = performance.now();
      const deltaTime = currentTime - lastTime;

      if (deltaTime >= targetFrameTime) {
        try {
          await this.demo.render();
          lastTime = currentTime;
        } catch (error) {
          console.error('Render error:', error);
        }
      }

      this.animationId = setTimeout(renderFrame, 1);
    };

    await renderFrame();
  }

  private startPerformanceMonitoring(): void {
    this.performanceInterval = setInterval(async () => {
      try {
        const metrics = await this.demo.getPerformanceMetrics();

        console.log('📊 Performance Metrics:');
        console.log(`   FPS: ${metrics.fps.toFixed(1)}`);
        console.log(`   Frame Time: ${metrics.frameTime.toFixed(2)}ms`);
        console.log(`   Draw Calls: ${metrics.drawCalls}`);
        console.log(`   Triangles: ${metrics.triangles.toLocaleString()}`);
        console.log(`   Memory: ${(metrics.memoryUsage / (1024 * 1024)).toFixed(1)}MB`);
        console.log(`   SIMD Speedup: ${metrics.simdSpeedup.toFixed(1)}x`);
        console.log('');

        // Warn about performance issues
        if (metrics.fps < this.config.targetFPS * 0.8) {
          console.warn(`⚠️  Low FPS detected: ${metrics.fps.toFixed(1)} (target: ${this.config.targetFPS})`);
        }

      } catch (error) {
        console.error('Performance monitoring error:', error);
      }
    }, 2000); // Every 2 seconds
  }

  private async runForDuration(duration: number): Promise<void> {
    return new Promise((resolve) => {
      setTimeout(() => {
        this.stop();
        resolve();
      }, duration);
    });
  }

  async stop(): Promise<void> {
    console.log('🛑 Stopping demo...');

    // Clear intervals and timeouts
    if (this.animationId) {
      clearTimeout(this.animationId);
      this.animationId = 0;
    }

    if (this.performanceInterval) {
      clearInterval(this.performanceInterval);
      this.performanceInterval = 0;
    }

    // Cleanup demo
    this.demo.cleanup();

    console.log('✅ Demo stopped successfully');
  }

  handleResize(width: number, height: number): void {
    this.config.canvasWidth = width;
    this.config.canvasHeight = height;

    if (this.canvas) {
      this.canvas.width = width;
      this.canvas.height = height;
    }

    this.demo.handleResize(width, height);
    console.log(`📐 Canvas resized to ${width}x${height}`);
  }
}

// CLI argument parsing
function parseArgs(): Partial<DemoConfig> {
  const args = Deno.args;
  const config: Partial<DemoConfig> = {};

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];

    switch (arg) {
      case '--width':
        config.canvasWidth = parseInt(args[++i] || '1024');
        break;
      case '--height':
        config.canvasHeight = parseInt(args[++i] || '768');
        break;
      case '--stress-test':
        config.enableStressTest = true;
        config.stressTestWidgets = parseInt(args[++i] || '1000');
        break;
      case '--fps':
        config.targetFPS = parseInt(args[++i] || '60');
        break;
      case '--no-perf':
        config.enablePerformanceMonitoring = false;
        break;
      case '--help':
        console.log(`
WebGPU GTK4 Widget Factory Demo

Usage: deno run --allow-read --allow-write --allow-net demo-deno.ts [options]

Options:
  --width <pixels>     Canvas width (default: 1024)
  --height <pixels>    Canvas height (default: 768)
  --stress-test <n>    Enable stress test with n widgets (default: 1000)
  --fps <number>       Target FPS (default: 60)
  --no-perf           Disable performance monitoring
  --help              Show this help

Examples:
  # Basic demo
  deno task demo

  # High resolution
  deno task demo --width 1920 --height 1080

  # Stress test
  deno task demo --stress-test 2000

  # 120 FPS target
  deno task demo --fps 120
        `);
        Deno.exit(0);
        break;
    }
  }

  return config;
}

// Main execution
async function main(): Promise<void> {
  try {
    console.log('🌐 WebGPU GTK4 Widget Factory Demo - Deno Edition');
    console.log('='.repeat(50));

    const config = parseArgs();
    const runner = new DemoRunner(config);

    // Handle graceful shutdown
    Deno.addSignalListener("SIGINT", () => {
      console.log('\n🛑 Received SIGINT, shutting down gracefully...');
      runner.stop().then(() => {
        Deno.exit(0);
      });
    });

    await runner.start();

    console.log('🎉 Demo completed successfully!');

  } catch (error) {
    console.error('❌ Demo failed:', error);
    Deno.exit(1);
  }
}

// Performance benchmarking
async function benchmark(): Promise<void> {
  console.log('🏁 Running WebGPU GTK4 performance benchmark...');

  const configs = [
    { canvasWidth: 800, canvasHeight: 600, stressTestWidgets: 100 },
    { canvasWidth: 1024, canvasHeight: 768, stressTestWidgets: 500 },
    { canvasWidth: 1920, canvasHeight: 1080, stressTestWidgets: 1000 },
    { canvasWidth: 1920, canvasHeight: 1080, stressTestWidgets: 2000 }
  ];

  for (const config of configs) {
    console.log(`\n📊 Testing ${config.canvasWidth}x${config.canvasHeight} with ${config.stressTestWidgets} widgets...`);

    const runner = new DemoRunner({
      ...config,
      enableStressTest: true,
      enablePerformanceMonitoring: false
    });

    try {
      await runner.start();

      // Run for 10 seconds
      await new Promise(resolve => setTimeout(resolve, 10000));

      const metrics = await runner.demo.getPerformanceMetrics();
      console.log(`   Results: ${metrics.fps.toFixed(1)} FPS, ${metrics.drawCalls} draw calls, ${metrics.simdSpeedup.toFixed(1)}x SIMD speedup`);

      await runner.stop();
    } catch (error) {
      console.error(`   Failed: ${error}`);
    }
  }

  console.log('\n🏆 Benchmark completed!');
}

// Export for module usage
export { DemoRunner, type DemoConfig };

// Run if called directly
if (import.meta.main) {
  if (Deno.args.includes('--benchmark')) {
    await benchmark();
  } else {
    await main();
  }
}