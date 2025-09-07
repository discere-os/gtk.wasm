import { test, expect } from '@playwright/test';

test.describe('WebGPU Initialization', () => {
  test.beforeEach(async ({ page }) => {
    // Navigate to the WebGPU hybrid demo
    await page.goto('http://127.0.0.1:8765/webgpu-hybrid-demo.html');
    // Wait a moment for page to load
    await page.waitForTimeout(1000);
  });

  test('should detect WebGPU availability', async ({ page }) => {
    const webgpuSupported = await page.evaluate(async () => {
      if (!navigator.gpu) {
        return { supported: false, error: 'navigator.gpu not available' };
      }
      
      try {
        const adapter = await navigator.gpu.requestAdapter();
        if (!adapter) {
          return { supported: false, error: 'No WebGPU adapter found' };
        }
        
        const device = await adapter.requestDevice();
        return { 
          supported: true, 
          adapter: adapter.info || 'Adapter available',
          deviceLabel: device.label || 'Device available'
        };
      } catch (error) {
        return { supported: false, error: error.message };
      }
    });

    console.log('WebGPU Support Result:', webgpuSupported);
    
    // The test should pass whether WebGPU is supported or falls back to Canvas 2D
    expect(webgpuSupported).toBeDefined();
    expect(typeof webgpuSupported.supported).toBe('boolean');
    
    if (webgpuSupported.supported) {
      console.log('✅ WebGPU is working!');
      expect(webgpuSupported.adapter).toBeDefined();
      expect(webgpuSupported.deviceLabel).toBeDefined();
    } else {
      console.log('🎨 Falling back to Canvas 2D:', webgpuSupported.error);
      expect(webgpuSupported.error).toBeDefined();
    }
  });

  test('should initialize GTK WebGPU demo', async ({ page }) => {
    // Wait for the demo to load
    await page.waitForSelector('#gtkCanvas', { timeout: 30000 });
    
    // Check if the demo initialized
    const demoStatus = await page.evaluate(() => {
      return {
        canvasExists: !!document.getElementById('gtkCanvas'),
        hasGTKApp: typeof window.gtkApp !== 'undefined',
        rendererInfo: document.querySelector('#renderer-info')?.textContent || 'Unknown',
        widgetCount: document.querySelector('#widget-count')?.textContent || '0'
      };
    });

    expect(demoStatus.canvasExists).toBe(true);
    expect(demoStatus.hasGTKApp).toBe(true);
    expect(demoStatus.rendererInfo).toContain('Renderer:');
    expect(demoStatus.widgetCount).toBeDefined();
    
    console.log('Demo Status:', demoStatus);
  });

  test('should render performance metrics', async ({ page }) => {
    // Wait for performance metrics to appear
    await page.waitForSelector('#performance-metrics', { timeout: 30000 });
    
    // Wait a bit for metrics to update
    await page.waitForTimeout(2000);
    
    const performanceData = await page.evaluate(() => {
      const fpsElement = document.querySelector('#fps');
      const frameTimeElement = document.querySelector('#frame-time');
      const widgetCountElement = document.querySelector('#widget-count');
      
      return {
        fps: fpsElement?.textContent || 'N/A',
        frameTime: frameTimeElement?.textContent || 'N/A',
        widgetCount: widgetCountElement?.textContent || 'N/A'
      };
    });

    expect(performanceData.fps).toMatch(/\d+(\.\d+)?\s*FPS/);
    expect(performanceData.frameTime).toMatch(/\d+(\.\d+)?\s*ms/);
    expect(performanceData.widgetCount).toMatch(/\d+/);
    
    console.log('Performance Metrics:', performanceData);
  });

  test('should add and render widgets', async ({ page }) => {
    // Wait for demo to be ready
    await page.waitForSelector('#addButton', { timeout: 30000 });
    await page.waitForTimeout(1000);

    // Add a button widget
    await page.click('#addButton');
    await page.waitForTimeout(500);

    // Add a label widget  
    await page.click('#addLabel');
    await page.waitForTimeout(500);

    // Check widget count updated
    const widgetCount = await page.textContent('#widget-count');
    expect(widgetCount).toMatch(/[2-9]/); // Should be at least 2 widgets

    // Take screenshot to verify visual output
    await page.screenshot({ path: 'test-results/widget-rendering.png' });

    // Verify canvas has content (not blank)
    const canvasData = await page.evaluate(() => {
      const canvas = document.getElementById('gtkCanvas');
      const ctx = canvas.getContext('2d');
      const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
      
      // Check if canvas has any non-transparent pixels
      let hasContent = false;
      for (let i = 3; i < imageData.data.length; i += 4) {
        if (imageData.data[i] > 0) { // Alpha channel > 0
          hasContent = true;
          break;
        }
      }
      
      return {
        hasContent,
        width: canvas.width,
        height: canvas.height
      };
    });

    expect(canvasData.hasContent).toBe(true);
    expect(canvasData.width).toBeGreaterThan(0);
    expect(canvasData.height).toBeGreaterThan(0);
    
    console.log('Canvas Data:', canvasData);
  });

  test('should clear all widgets', async ({ page }) => {
    // Wait for demo to be ready
    await page.waitForSelector('#addButton', { timeout: 30000 });

    // Add some widgets first
    await page.click('#addButton');
    await page.click('#addLabel');
    await page.waitForTimeout(1000);

    // Verify widgets were added
    let widgetCount = await page.textContent('#widget-count');
    expect(widgetCount).toMatch(/[2-9]/);

    // Clear all widgets
    await page.click('#clearAll');
    await page.waitForTimeout(500);

    // Verify widgets were cleared
    widgetCount = await page.textContent('#widget-count');
    expect(widgetCount).toMatch(/0/);
  });
});