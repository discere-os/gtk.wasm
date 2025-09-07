import { defineConfig, devices } from '@playwright/test';

/**
 * Playwright configuration for gtk.wasm WebGPU testing
 * UI Tier 3 - WebGPU hardware acceleration testing
 */
export default defineConfig({
  testDir: './test',
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 1 : undefined, // Reduced workers for GPU testing
  
  reporter: [
    ['html', { outputFolder: 'playwright-report' }],
    ['json', { outputFile: 'test-results/results.json' }],
    ['junit', { outputFile: 'test-results/junit.xml' }],
    process.env.CI ? ['github'] : ['list']
  ],

  timeout: 90000, // Increased timeout for GPU initialization
  expect: { timeout: 45000 },

  use: {
    // Required headers for SharedArrayBuffer, WASM, and WebGPU
    extraHTTPHeaders: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Resource-Policy': 'cross-origin',
      'Cache-Control': 'no-cache',
      'Permissions-Policy': 'webgpu=*'
    },
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
    video: 'retain-on-failure'
  },

  projects: [
    // WebGPU Chrome testing with all experimental features
    {
      name: 'gtk-webgpu-chrome',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: [
            // WebGPU support flags
            '--enable-unsafe-webgpu',
            '--enable-features=Vulkan,WebGPU',
            '--use-vulkan',
            '--disable-vulkan-fallback-to-gl-for-testing',
            
            // WASM and SIMD support
            '--enable-features=WebAssemblySimd,WebAssemblyThreads',
            '--enable-wasm-simd',
            '--enable-experimental-wasm-features',
            
            // GPU acceleration
            '--enable-gpu-rasterization',
            '--enable-zero-copy',
            '--enable-hardware-overlays',
            '--use-gl=angle',
            
            // Testing flags
            '--disable-web-security', // For local testing
            '--disable-features=VizDisplayCompositor',
            '--no-sandbox',
            '--disable-setuid-sandbox',
            '--disable-dev-shm-usage',
            
            // WebGPU debugging
            '--enable-webgpu-developer-features',
            '--webgpu-adapter-name=*'
          ]
        }
      },
      testMatch: ['**/webgpu/**/*.spec.js', '**/professional/**/*.spec.js', '**/graphics/**/*.spec.js']
    },

    // Firefox WebGPU testing (experimental)
    {
      name: 'gtk-webgpu-firefox',
      use: {
        ...devices['Desktop Firefox'],
        launchOptions: {
          firefoxUserPrefs: {
            // WebGPU experimental support
            'dom.webgpu.enabled': true,
            'gfx.webrender.all': true,
            'gfx.webrender.enabled': true,
            
            // WASM support
            'javascript.options.wasm_simd': true,
            'javascript.options.wasm_exceptions': true,
            'javascript.options.wasm_function_references': true,
            
            // COOP/COEP for SharedArrayBuffer
            'dom.postMessage.sharedArrayBuffer.bypassCOOP_COEP.insecure.enabled': true
          }
        }
      },
      testMatch: '**/webgpu/**/*.spec.js'
    },

    // Canvas 2D fallback testing
    {
      name: 'gtk-canvas-fallback',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: [
            // Disable WebGPU to test Canvas 2D fallback
            '--disable-features=WebGPU',
            '--enable-features=WebAssemblySimd',
            '--enable-wasm-simd',
            '--disable-web-security'
          ]
        }
      },
      testMatch: '**/canvas/**/*.spec.js'
    },

    // GTK widget rendering tests
    {
      name: 'gtk-widget-rendering',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: [
            '--enable-unsafe-webgpu',
            '--enable-features=Vulkan,WebGPU,WebAssemblySimd',
            '--use-vulkan',
            '--enable-wasm-simd',
            '--disable-web-security',
            '--enable-webgpu-developer-features'
          ]
        }
      },
      testMatch: '**/widgets/**/*.spec.js',
      timeout: 120000 // Extended timeout for widget creation
    },

    // Performance testing with WebGPU
    {
      name: 'gtk-webgpu-performance',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: [
            '--enable-unsafe-webgpu',
            '--enable-features=Vulkan,WebGPU,WebAssemblySimd,WebAssemblyThreads',
            '--use-vulkan',
            '--enable-wasm-simd',
            '--enable-gpu-rasterization',
            '--enable-zero-copy',
            '--no-sandbox',
            '--disable-setuid-sandbox',
            '--disable-dev-shm-usage',
            '--disable-web-security'
          ]
        }
      },
      testMatch: '**/performance/**/*.spec.js',
      timeout: 180000,
      retries: 0 // No retries for performance tests
    },

    // Mobile WebGPU testing (where supported)
    {
      name: 'gtk-mobile-webgpu',
      use: { 
        ...devices['Pixel 5'],
        launchOptions: {
          args: [
            '--enable-unsafe-webgpu',
            '--enable-features=WebGPU,WebAssemblySimd',
            '--disable-web-security'
          ]
        }
      },
      testMatch: '**/mobile/**/*.spec.js'
    }
  ],

  // Web server for serving WASM files and test pages
  webServer: {
    command: 'cd examples && python3 -m http.server 8765',
    port: 8765,
    reuseExistingServer: !process.env.CI,
    timeout: 30000
  },

  // Global setup and teardown
  globalSetup: './test/utils/global-setup.js',
  globalTeardown: './test/utils/global-teardown.js'
});