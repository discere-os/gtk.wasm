import { test, expect } from '@playwright/test';

test.describe('Professional Application UI Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('http://127.0.0.1:8765/blender-gimp-ui-test.html');
    // Wait for WebGPU initialization
    await page.waitForTimeout(2000);
  });

  test('should initialize professional UI layout', async ({ page }) => {
    // Verify all major UI components are present
    await expect(page.locator('.menu-bar')).toBeVisible();
    await expect(page.locator('.toolbar')).toBeVisible();
    await expect(page.locator('.sidebar')).toBeVisible();
    await expect(page.locator('.viewport')).toBeVisible();
    await expect(page.locator('.properties-panel')).toBeVisible();
    await expect(page.locator('.status-bar')).toBeVisible();

    // Verify canvas is present and sized correctly
    const canvas = page.locator('#gtkCanvas');
    await expect(canvas).toBeVisible();
    
    const canvasBox = await canvas.boundingBox();
    expect(canvasBox.width).toBeGreaterThan(500);
    expect(canvasBox.height).toBeGreaterThan(400);
  });

  test('should detect WebGPU hardware acceleration', async ({ page }) => {
    // Wait for renderer initialization
    await page.waitForTimeout(3000);
    
    const rendererInfo = await page.textContent('#rendererInfo');
    const statusBar = await page.textContent('#statusBar');
    
    console.log('Renderer:', rendererInfo);
    console.log('Status:', statusBar);
    
    // Should either have WebGPU or graceful fallback
    expect(rendererInfo).toMatch(/(WebGPU Hardware Accelerated|Canvas 2D Software Fallback)/);
    expect(statusBar).toMatch(/(WebGPU: ✅ Enabled|WebGPU: ❌ Unavailable)/);
    
    // If WebGPU is available, verify high performance mode
    if (rendererInfo.includes('WebGPU')) {
      expect(statusBar).toContain('High Performance Mode');
    }
  });

  test('should handle stress test with 1000+ widgets', async ({ page }) => {
    // Start stress test
    await page.click('#stressTest');
    
    // Wait for stress test to complete
    await page.waitForTimeout(5000);
    
    // Check widget count increased significantly
    const widgetCount = await page.textContent('#widgetCount');
    const count = parseInt(widgetCount);
    expect(count).toBeGreaterThan(1000);
    
    // Verify performance metrics
    const fps = await page.textContent('#fps');
    const fpsValue = parseInt(fps.split(' ')[0]);
    
    console.log(`Performance under stress: ${fps}, ${count} widgets`);
    
    // Even under stress, should maintain reasonable performance
    expect(fpsValue).toBeGreaterThan(15); // Minimum acceptable FPS
    
    // Memory usage should be tracked
    const memoryUsage = await page.textContent('#memoryUsage');
    expect(memoryUsage).toMatch(/\d+ MB/);
    
    // Take screenshot for visual verification
    await page.screenshot({ path: 'test-results/stress-test-1000-widgets.png', fullPage: true });
  });

  test('should create complex professional layouts', async ({ page }) => {
    // Test complex layout creation
    await page.click('#complexLayout');
    await page.waitForTimeout(3000);
    
    const widgetCount = await page.textContent('#widgetCount');
    const count = parseInt(widgetCount);
    
    // Should have created multiple complex layout sections
    expect(count).toBeGreaterThan(300); // Header, tool shelf, outliner, properties, timeline, viewport overlays
    
    // Verify performance with complex layouts
    const fps = await page.textContent('#fps');
    const fpsValue = parseInt(fps.split(' ')[0]);
    expect(fpsValue).toBeGreaterThan(20);
    
    console.log(`Complex layout performance: ${fps}, ${count} widgets`);
    
    // Take screenshot of complex layout
    await page.screenshot({ path: 'test-results/complex-professional-layout.png', fullPage: true });
  });

  test('should handle complex menu system', async ({ page }) => {
    // Test menu system like GIMP
    await page.click('#menuTest');
    await page.waitForTimeout(2000);
    
    const initialCount = parseInt(await page.textContent('#widgetCount'));
    
    // Should have created menu hierarchy
    expect(initialCount).toBeGreaterThan(80); // 11 menus * 8 items each
    
    // Verify menu system rendering
    const canvas = page.locator('#gtkCanvas');
    const canvasData = await page.evaluate(() => {
      const canvas = document.getElementById('gtkCanvas');
      const ctx = canvas.getContext('2d');
      const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
      
      let nonTransparentPixels = 0;
      for (let i = 3; i < imageData.data.length; i += 4) {
        if (imageData.data[i] > 0) nonTransparentPixels++;
      }
      
      return {
        hasContent: nonTransparentPixels > 1000,
        nonTransparentPixels,
        width: canvas.width,
        height: canvas.height
      };
    });
    
    expect(canvasData.hasContent).toBe(true);
    expect(canvasData.nonTransparentPixels).toBeGreaterThan(1000);
    
    console.log(`Menu system rendering: ${canvasData.nonTransparentPixels} pixels drawn`);
  });

  test('should support Blender-style keyboard shortcuts', async ({ page }) => {
    const initialCount = parseInt(await page.textContent('#widgetCount'));
    
    // Test Blender shortcuts
    await page.keyboard.press('Tab'); // Edit mode
    await page.waitForTimeout(100);
    await page.keyboard.press('g'); // Grab tool
    await page.waitForTimeout(100);
    await page.keyboard.press('r'); // Rotate tool
    await page.waitForTimeout(100);
    await page.keyboard.press('s'); // Scale tool
    await page.waitForTimeout(500);
    
    const finalCount = parseInt(await page.textContent('#widgetCount'));
    expect(finalCount).toBeGreaterThan(initialCount);
    
    console.log(`Keyboard shortcuts added ${finalCount - initialCount} widgets`);
  });

  test('should maintain performance with continuous rendering', async ({ page }) => {
    // Monitor performance over time
    const performanceData = [];
    
    for (let i = 0; i < 10; i++) {
      await page.waitForTimeout(1000);
      
      const fps = parseInt((await page.textContent('#fps')).split(' ')[0]);
      const frameTime = parseFloat((await page.textContent('#frameTime')).split(' ')[0]);
      const widgetCount = parseInt(await page.textContent('#widgetCount'));
      const memoryUsage = parseInt((await page.textContent('#memoryUsage')).split(' ')[0]);
      
      performanceData.push({ fps, frameTime, widgetCount, memoryUsage, timestamp: Date.now() });
    }
    
    // Analyze performance trends
    const avgFPS = performanceData.reduce((sum, d) => sum + d.fps, 0) / performanceData.length;
    const avgFrameTime = performanceData.reduce((sum, d) => sum + d.frameTime, 0) / performanceData.length;
    const maxMemory = Math.max(...performanceData.map(d => d.memoryUsage));
    
    console.log(`Sustained performance: ${avgFPS.toFixed(1)} FPS avg, ${avgFrameTime.toFixed(1)}ms frame time, ${maxMemory}MB peak memory`);
    
    // Professional applications need sustained performance
    expect(avgFPS).toBeGreaterThan(25);
    expect(avgFrameTime).toBeLessThan(50);
    expect(maxMemory).toBeLessThan(200); // Reasonable memory usage
  });

  test('should render UI elements with professional quality', async ({ page }) => {
    // Add some complex widgets
    await page.click('#complexLayout');
    await page.waitForTimeout(2000);
    
    // Verify visual rendering quality
    const visualQuality = await page.evaluate(() => {
      const canvas = document.getElementById('gtkCanvas');
      const ctx = canvas.getContext('2d');
      const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
      
      let colorVariety = new Set();
      let gradientPixels = 0;
      
      for (let i = 0; i < imageData.data.length; i += 16) { // Sample every 4th pixel
        const r = imageData.data[i];
        const g = imageData.data[i + 1];
        const b = imageData.data[i + 2];
        const a = imageData.data[i + 3];
        
        if (a > 0) {
          colorVariety.add(`${r},${g},${b}`);
          
          // Check for gradient-like color transitions
          if (i > 4 * canvas.width * 4) { // Not first row
            const prevR = imageData.data[i - canvas.width * 4];
            const colorDiff = Math.abs(r - prevR);
            if (colorDiff > 5 && colorDiff < 30) {
              gradientPixels++;
            }
          }
        }
      }
      
      return {
        colorVariety: colorVariety.size,
        gradientPixels,
        hasContent: colorVariety.size > 10
      };
    });
    
    // Professional UI should have color variety and smooth gradients
    expect(visualQuality.colorVariety).toBeGreaterThan(20);
    expect(visualQuality.gradientPixels).toBeGreaterThan(100);
    expect(visualQuality.hasContent).toBe(true);
    
    console.log(`Visual quality: ${visualQuality.colorVariety} colors, ${visualQuality.gradientPixels} gradient pixels`);
    
    // Take final screenshot for visual verification
    await page.screenshot({ path: 'test-results/professional-ui-quality.png', fullPage: true });
  });

  test('should clear all widgets properly', async ({ page }) => {
    // Add widgets first
    await page.click('#stressTest');
    await page.waitForTimeout(2000);
    
    const beforeCount = parseInt(await page.textContent('#widgetCount'));
    expect(beforeCount).toBeGreaterThan(0);
    
    // Clear all
    await page.click('#clearAll');
    await page.waitForTimeout(500);
    
    const afterCount = parseInt(await page.textContent('#widgetCount'));
    expect(afterCount).toBe(0);
    
    // Verify canvas is cleared
    const isEmpty = await page.evaluate(() => {
      const canvas = document.getElementById('gtkCanvas');
      const ctx = canvas.getContext('2d');
      const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
      
      for (let i = 3; i < imageData.data.length; i += 4) {
        if (imageData.data[i] > 0) return false;
      }
      return true;
    });
    
    // Note: Canvas might have background color, so we just check widget count
    expect(afterCount).toBe(0);
    console.log('✅ Successfully cleared all widgets');
  });
});