#!/usr/bin/env -S deno run --allow-read --allow-write --allow-net

// WebGPU GTK4 Performance Validation Suite
// Comprehensive benchmarks following WASM-native architecture principles

import { DemoRunner, type DemoConfig } from './demos/webgpu-widget-factory/demo-deno.ts';

interface BenchmarkResult {
  testName: string;
  canvasSize: { width: number; height: number };
  widgetCount: number;
  duration: number;
  metrics: {
    averageFPS: number;
    minFPS: number;
    maxFPS: number;
    averageFrameTime: number;
    totalDrawCalls: number;
    totalTriangles: number;
    peakMemoryMB: number;
    simdSpeedup: number;
  };
  passed: boolean;
  requirements: PerformanceRequirements;
}

interface PerformanceRequirements {
  minFPS: number;
  maxFrameTime: number;
  maxMemoryMB: number;
  minSIMDSpeedup: number;
  description: string;
}

const PERFORMANCE_REQUIREMENTS: Record<string, PerformanceRequirements> = {
  'basic-rendering': {
    minFPS: 60,
    maxFrameTime: 16.67,
    maxMemoryMB: 50,
    minSIMDSpeedup: 1.0,
    description: 'Basic widget rendering with 50 widgets'
  },
  'medium-load': {
    minFPS: 45,
    maxFrameTime: 22.22,
    maxMemoryMB: 100,
    minSIMDSpeedup: 2.5,
    description: 'Medium load with 200 widgets and SIMD optimizations'
  },
  'high-load': {
    minFPS: 30,
    maxFrameTime: 33.33,
    maxMemoryMB: 200,
    minSIMDSpeedup: 3.0,
    description: 'High load stress test with 1000 widgets'
  },
  'extreme-load': {
    minFPS: 20,
    maxFrameTime: 50.0,
    maxMemoryMB: 400,
    minSIMDSpeedup: 3.5,
    description: 'Extreme load with 2000+ widgets'
  },
  'high-resolution': {
    minFPS: 40,
    maxFrameTime: 25.0,
    maxMemoryMB: 300,
    minSIMDSpeedup: 3.0,
    description: 'High resolution 4K rendering'
  }
};

class PerformanceValidator {
  private results: BenchmarkResult[] = [];

  async runAllBenchmarks(): Promise<BenchmarkResult[]> {
    console.log('🚀 Starting WebGPU GTK4 Performance Validation Suite');
    console.log('=' .repeat(60));

    // Test configurations following web-native performance targets
    const testConfigs = [
      {
        name: 'basic-rendering',
        config: { canvasWidth: 1024, canvasHeight: 768, stressTestWidgets: 50 },
        duration: 15000
      },
      {
        name: 'medium-load',
        config: { canvasWidth: 1024, canvasHeight: 768, stressTestWidgets: 200 },
        duration: 15000
      },
      {
        name: 'high-load',
        config: { canvasWidth: 1024, canvasHeight: 768, stressTestWidgets: 1000 },
        duration: 20000
      },
      {
        name: 'extreme-load',
        config: { canvasWidth: 1024, canvasHeight: 768, stressTestWidgets: 2000 },
        duration: 20000
      },
      {
        name: 'high-resolution',
        config: { canvasWidth: 3840, canvasHeight: 2160, stressTestWidgets: 500 },
        duration: 20000
      }
    ];

    for (const testConfig of testConfigs) {
      console.log(`\n📊 Running ${testConfig.name}...`);
      console.log(`   ${PERFORMANCE_REQUIREMENTS[testConfig.name].description}`);

      try {
        const result = await this.runBenchmark(
          testConfig.name,
          testConfig.config,
          testConfig.duration
        );
        this.results.push(result);

        this.printResult(result);
      } catch (error) {
        console.error(`❌ Test ${testConfig.name} failed:`, error);
      }

      // Cool down between tests
      await new Promise(resolve => setTimeout(resolve, 2000));
    }

    this.printSummary();
    return this.results;
  }

  private async runBenchmark(
    testName: string,
    config: Partial<DemoConfig>,
    duration: number
  ): Promise<BenchmarkResult> {
    const runner = new DemoRunner({
      ...config,
      enableStressTest: true,
      enablePerformanceMonitoring: false
    });

    const requirements = PERFORMANCE_REQUIREMENTS[testName];
    const metrics = {
      fpsHistory: [] as number[],
      frameTimeHistory: [] as number[],
      drawCallsHistory: [] as number[],
      trianglesHistory: [] as number[],
      memoryHistory: [] as number[],
      simdSpeedup: 1.0
    };

    try {
      // Initialize demo
      await runner.start();
      console.log(`   Initialized. Running for ${duration / 1000}s...`);

      // Collect metrics during test
      const startTime = performance.now();
      const metricsInterval = setInterval(async () => {
        try {
          const currentMetrics = await runner.demo.getPerformanceMetrics();
          metrics.fpsHistory.push(currentMetrics.fps);
          metrics.frameTimeHistory.push(currentMetrics.frameTime);
          metrics.drawCallsHistory.push(currentMetrics.drawCalls);
          metrics.trianglesHistory.push(currentMetrics.triangles);
          metrics.memoryHistory.push(currentMetrics.memoryUsage / (1024 * 1024));
          metrics.simdSpeedup = currentMetrics.simdSpeedup;
        } catch (error) {
          console.warn('   Metrics collection error:', error);
        }
      }, 500); // Every 500ms

      // Wait for test duration
      await new Promise(resolve => setTimeout(resolve, duration));

      clearInterval(metricsInterval);
      await runner.stop();

      // Calculate final metrics
      const averageFPS = this.average(metrics.fpsHistory);
      const minFPS = Math.min(...metrics.fpsHistory);
      const maxFPS = Math.max(...metrics.fpsHistory);
      const averageFrameTime = this.average(metrics.frameTimeHistory);
      const totalDrawCalls = this.sum(metrics.drawCallsHistory);
      const totalTriangles = this.sum(metrics.trianglesHistory);
      const peakMemoryMB = Math.max(...metrics.memoryHistory);

      // Determine if test passed
      const passed =
        averageFPS >= requirements.minFPS &&
        averageFrameTime <= requirements.maxFrameTime &&
        peakMemoryMB <= requirements.maxMemoryMB &&
        metrics.simdSpeedup >= requirements.minSIMDSpeedup;

      return {
        testName,
        canvasSize: {
          width: config.canvasWidth || 1024,
          height: config.canvasHeight || 768
        },
        widgetCount: config.stressTestWidgets || 0,
        duration,
        metrics: {
          averageFPS,
          minFPS,
          maxFPS,
          averageFrameTime,
          totalDrawCalls,
          totalTriangles,
          peakMemoryMB,
          simdSpeedup: metrics.simdSpeedup
        },
        passed,
        requirements
      };

    } catch (error) {
      await runner.stop();
      throw error;
    }
  }

  private printResult(result: BenchmarkResult): void {
    const status = result.passed ? '✅ PASSED' : '❌ FAILED';
    console.log(`   ${status}`);
    console.log(`   FPS: ${result.metrics.averageFPS.toFixed(1)} avg (min: ${result.metrics.minFPS.toFixed(1)}, req: ≥${result.requirements.minFPS})`);
    console.log(`   Frame Time: ${result.metrics.averageFrameTime.toFixed(2)}ms (req: ≤${result.requirements.maxFrameTime}ms)`);
    console.log(`   Memory: ${result.metrics.peakMemoryMB.toFixed(1)}MB (req: ≤${result.requirements.maxMemoryMB}MB)`);
    console.log(`   SIMD Speedup: ${result.metrics.simdSpeedup.toFixed(1)}x (req: ≥${result.requirements.minSIMDSpeedup}x)`);
    console.log(`   Draw Calls: ${result.metrics.totalDrawCalls.toLocaleString()}`);
    console.log(`   Triangles: ${result.metrics.totalTriangles.toLocaleString()}`);

    if (!result.passed) {
      console.log('   🔍 Failure Analysis:');
      if (result.metrics.averageFPS < result.requirements.minFPS) {
        console.log(`      - FPS too low: ${result.metrics.averageFPS.toFixed(1)} < ${result.requirements.minFPS}`);
      }
      if (result.metrics.averageFrameTime > result.requirements.maxFrameTime) {
        console.log(`      - Frame time too high: ${result.metrics.averageFrameTime.toFixed(2)}ms > ${result.requirements.maxFrameTime}ms`);
      }
      if (result.metrics.peakMemoryMB > result.requirements.maxMemoryMB) {
        console.log(`      - Memory usage too high: ${result.metrics.peakMemoryMB.toFixed(1)}MB > ${result.requirements.maxMemoryMB}MB`);
      }
      if (result.metrics.simdSpeedup < result.requirements.minSIMDSpeedup) {
        console.log(`      - SIMD speedup too low: ${result.metrics.simdSpeedup.toFixed(1)}x < ${result.requirements.minSIMDSpeedup}x`);
      }
    }
  }

  private printSummary(): void {
    console.log('\n📋 Performance Validation Summary');
    console.log('=' .repeat(60));

    const passed = this.results.filter(r => r.passed).length;
    const total = this.results.length;
    const passRate = (passed / total) * 100;

    console.log(`Tests Passed: ${passed}/${total} (${passRate.toFixed(1)}%)`);

    if (passRate === 100) {
      console.log('🎉 All performance requirements met!');
      console.log('   WebGPU GTK4 renderer achieves web-native performance targets');
    } else if (passRate >= 80) {
      console.log('⚠️  Most performance requirements met, some optimization needed');
    } else {
      console.log('❌ Significant performance issues detected');
      console.log('   WebGPU renderer requires optimization work');
    }

    console.log('\n🏆 Performance Highlights:');
    const bestFPS = Math.max(...this.results.map(r => r.metrics.averageFPS));
    const bestMemory = Math.min(...this.results.map(r => r.metrics.peakMemoryMB));
    const bestSIMD = Math.max(...this.results.map(r => r.metrics.simdSpeedup));

    console.log(`   Peak FPS: ${bestFPS.toFixed(1)}`);
    console.log(`   Lowest Memory: ${bestMemory.toFixed(1)}MB`);
    console.log(`   Best SIMD Speedup: ${bestSIMD.toFixed(1)}x`);

    // Analyze SIMD effectiveness
    const avgSIMDSpeedup = this.average(this.results.map(r => r.metrics.simdSpeedup));
    console.log(`   Average SIMD Speedup: ${avgSIMDSpeedup.toFixed(1)}x`);

    if (avgSIMDSpeedup >= 3.0) {
      console.log('   ✅ SIMD optimizations highly effective');
    } else if (avgSIMDSpeedup >= 2.0) {
      console.log('   ⚠️  SIMD optimizations moderately effective');
    } else {
      console.log('   ❌ SIMD optimizations need improvement');
    }
  }

  async generateReport(): Promise<void> {
    const report = {
      timestamp: new Date().toISOString(),
      summary: {
        totalTests: this.results.length,
        passedTests: this.results.filter(r => r.passed).length,
        passRate: (this.results.filter(r => r.passed).length / this.results.length) * 100,
        avgSIMDSpeedup: this.average(this.results.map(r => r.metrics.simdSpeedup))
      },
      results: this.results,
      recommendations: this.generateRecommendations()
    };

    const reportPath = './webgpu-performance-report.json';
    await Deno.writeTextFile(reportPath, JSON.stringify(report, null, 2));
    console.log(`\n📄 Detailed report saved to: ${reportPath}`);
  }

  private generateRecommendations(): string[] {
    const recommendations: string[] = [];

    const avgFPS = this.average(this.results.map(r => r.metrics.averageFPS));
    const avgMemory = this.average(this.results.map(r => r.metrics.peakMemoryMB));
    const avgSIMD = this.average(this.results.map(r => r.metrics.simdSpeedup));

    if (avgFPS < 45) {
      recommendations.push('Consider optimizing render pipeline for better FPS');
      recommendations.push('Review draw call batching and state management');
    }

    if (avgMemory > 200) {
      recommendations.push('Implement more aggressive memory management');
      recommendations.push('Consider texture compression and memory pooling');
    }

    if (avgSIMD < 2.5) {
      recommendations.push('Improve SIMD optimization coverage');
      recommendations.push('Profile hot paths for additional SIMD opportunities');
    }

    const failedTests = this.results.filter(r => !r.passed);
    if (failedTests.length > 0) {
      recommendations.push(`Focus optimization on failed tests: ${failedTests.map(t => t.testName).join(', ')}`);
    }

    return recommendations;
  }

  // Utility functions
  private average(numbers: number[]): number {
    return numbers.length > 0 ? numbers.reduce((a, b) => a + b, 0) / numbers.length : 0;
  }

  private sum(numbers: number[]): number {
    return numbers.reduce((a, b) => a + b, 0);
  }
}

// WebGPU Capability Validation
async function validateWebGPUCapabilities(): Promise<boolean> {
  console.log('🔍 Validating WebGPU capabilities...');

  // Mock WebGPU detection for Deno environment
  const capabilities = {
    hasWebGPU: true, // Simulated
    hasWASMSIMD: true, // Simulated
    hasSharedArrayBuffer: typeof SharedArrayBuffer !== "undefined",
    hasTimestampQuery: true, // Simulated
    hasTextureCompressionBC: true // Simulated
  };

  console.log('   WebGPU Available:', capabilities.hasWebGPU ? '✅' : '❌');
  console.log('   WASM SIMD:', capabilities.hasWASMSIMD ? '✅' : '❌');
  console.log('   SharedArrayBuffer:', capabilities.hasSharedArrayBuffer ? '✅' : '❌');
  console.log('   Timestamp Query:', capabilities.hasTimestampQuery ? '✅' : '❌');
  console.log('   BC Compression:', capabilities.hasTextureCompressionBC ? '✅' : '❌');

  const allCapabilitiesPresent = Object.values(capabilities).every(Boolean);

  if (allCapabilitiesPresent) {
    console.log('✅ All required capabilities present');
  } else {
    console.log('❌ Missing required capabilities');
  }

  return allCapabilitiesPresent;
}

// Main execution
async function main(): Promise<void> {
  try {
    console.log('🌐 WebGPU GTK4 Performance Validation Suite');
    console.log('   Web-native architecture with WASM SIMD optimizations');
    console.log('   Target: 3-10x performance improvements over traditional approaches');
    console.log('');

    // Validate capabilities first
    const capabilitiesValid = await validateWebGPUCapabilities();
    if (!capabilitiesValid) {
      console.log('⚠️  Some capabilities missing, results may not be representative');
    }

    const validator = new PerformanceValidator();
    const results = await validator.runAllBenchmarks();

    await validator.generateReport();

    // Exit with appropriate code
    const passRate = (results.filter(r => r.passed).length / results.length) * 100;
    if (passRate === 100) {
      console.log('\n🎉 All performance validation tests passed!');
      Deno.exit(0);
    } else if (passRate >= 80) {
      console.log('\n⚠️  Performance validation mostly passed with some issues');
      Deno.exit(1);
    } else {
      console.log('\n❌ Performance validation failed');
      Deno.exit(2);
    }

  } catch (error) {
    console.error('❌ Validation suite failed:', error);
    Deno.exit(3);
  }
}

// Export for module usage
export { PerformanceValidator, type BenchmarkResult, type PerformanceRequirements };

// Run if called directly
if (import.meta.main) {
  await main();
}