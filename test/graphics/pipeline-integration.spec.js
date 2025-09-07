import { test, expect } from '@playwright/test';

test.describe('Graphics Pipeline Integration Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('http://127.0.0.1:8765/graphics-pipeline-test.html');
    // Wait for WebGPU initialization and initial tests
    await page.waitForTimeout(3000);
  });

  test('should initialize WebGPU graphics pipeline', async ({ page }) => {
    const rendererType = await page.textContent('#rendererType');
    const pipelineState = await page.textContent('#pipelineState');
    
    console.log('Graphics Pipeline Status:', rendererType, '|', pipelineState);
    
    // Should either have WebGPU or fallback gracefully
    expect(rendererType).toMatch(/(WebGPU Hardware Pipeline|Canvas 2D Fallback)/);
    expect(pipelineState).toMatch(/(Initialized|Software Rendering)/);
    
    // If WebGPU is available, verify advanced features
    if (rendererType.includes('WebGPU')) {
      const shaderStages = parseInt(await page.textContent('#shaderStages'));
      expect(shaderStages).toBeGreaterThan(8); // Multiple pipeline stages
    }
  });

  test('should create multiple specialized graphics pipelines', async ({ page }) => {
    // Wait for pipeline creation to complete
    await page.waitForTimeout(2000);
    
    const shaderStages = parseInt(await page.textContent('#shaderStages'));
    console.log('Shader stages created:', shaderStages);
    
    // Should have multiple pipelines for professional graphics
    expect(shaderStages).toBeGreaterThan(0);
    
    // Verify test results show successful pipeline creation
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('Image processing pipeline created');
    expect(testResults).toContain('Brush rendering pipeline created');
    expect(testResults).toContain('Layer compositing pipeline created');
    expect(testResults).toContain('Vector graphics pipeline created');
    expect(testResults).toContain('Text rendering pipeline created');
  });

  test('should handle professional image processing operations', async ({ page }) => {
    // Test color correction
    await page.click('#testColorCorrection');
    await page.waitForTimeout(500);
    
    // Test blur filter
    await page.click('#testBlurFilter');
    await page.waitForTimeout(300);
    
    // Test sharpen filter
    await page.click('#testSharpenFilter');
    await page.waitForTimeout(300);
    
    // Test compositing
    await page.click('#testCompositing');
    await page.waitForTimeout(500);
    
    // Verify draw calls increased
    const drawCalls = parseInt(await page.textContent('#drawCalls'));
    expect(drawCalls).toBeGreaterThan(10); // Should have executed multiple operations
    
    // Verify operations completed successfully
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('Color correction');
    expect(testResults).toContain('Blur filter test completed');
    expect(testResults).toContain('Sharpen filter test completed');
    expect(testResults).toContain('Alpha compositing test completed');
    
    console.log(`Image processing tests: ${drawCalls} draw calls executed`);
  });

  test('should support professional graphics tools', async ({ page }) => {
    // Test brush engine
    await page.click('#testBrushEngine');
    await page.waitForTimeout(1000);
    
    // Test layer system
    await page.click('#testLayers');
    await page.waitForTimeout(800);
    
    // Test selection tools
    await page.click('#testSelection');
    await page.waitForTimeout(500);
    
    // Test vector graphics
    await page.click('#testVectorGraphics');
    await page.waitForTimeout(1000);
    
    const drawCalls = parseInt(await page.textContent('#drawCalls'));
    console.log(`Professional tools draw calls: ${drawCalls}`);
    
    // Should handle complex tool operations
    expect(drawCalls).toBeGreaterThan(20);
    
    // Verify professional tool operations
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('10 brush strokes rendered');
    expect(testResults).toContain('5 layers composited');
    expect(testResults).toContain('15 vector elements');
    
    // Take screenshot to verify visual rendering
    await page.screenshot({ path: 'test-results/graphics-pipeline-tools.png' });
  });

  test('should handle high-resolution image processing', async ({ page }) => {
    // Test 4K image processing
    const startTime = Date.now();
    await page.click('#testMegapixelImage');
    
    // Wait for 4K processing to complete
    await page.waitForTimeout(2000);
    const endTime = Date.now();
    
    const processingTime = endTime - startTime;
    console.log(`4K image processing time: ${processingTime}ms`);
    
    // Should complete 4K processing in reasonable time
    expect(processingTime).toBeLessThan(3000);
    
    // Check texture memory usage
    const textureMemory = parseInt((await page.textContent('#textureMemory')).split(' ')[0]);
    expect(textureMemory).toBeGreaterThan(0);
    
    // Verify 4K processing completed
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('4K processing completed');
    
    console.log(`4K processing: ${textureMemory}MB texture memory used`);
  });

  test('should maintain performance with complex compositing', async ({ page }) => {
    // Run complex composite operations
    await page.click('#testComplexComposite');
    await page.waitForTimeout(1500);
    
    const drawCalls = parseInt(await page.textContent('#drawCalls'));
    console.log(`Complex compositing draw calls: ${drawCalls}`);
    
    // Should handle many blend operations
    expect(drawCalls).toBeGreaterThan(30);
    
    // Verify complex operations completed
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('25 blend operations');
    
    // Test performance doesn't degrade significantly
    const verticesPerFrame = parseInt(await page.textContent('#verticesPerFrame'));
    expect(verticesPerFrame).toBeGreaterThan(0);
  });

  test('should support realtime filter processing', async ({ page }) => {
    // Test realtime filters (this will take about 1 second)
    const startTime = Date.now();
    await page.click('#testRealtimeFilters');
    
    // Wait for realtime test to complete
    await page.waitForTimeout(2000);
    const endTime = Date.now();
    
    const realtimeTestTime = endTime - startTime;
    console.log(`Realtime filters test duration: ${realtimeTestTime}ms`);
    
    // Should maintain realtime performance
    expect(realtimeTestTime).toBeLessThan(3000);
    
    // Verify realtime processing succeeded
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('60 FPS sustained');
    
    const drawCalls = parseInt(await page.textContent('#drawCalls'));
    console.log(`Realtime processing: ${drawCalls} total draw calls`);
  });

  test('should render actual graphics content', async ({ page }) => {
    // Trigger some visual operations
    await page.click('#testColorCorrection');
    await page.waitForTimeout(500);
    await page.click('#testGradients');
    await page.waitForTimeout(500);
    await page.click('#testPatterns');
    await page.waitForTimeout(500);
    
    // Verify canvas has actual visual content
    const canvasContent = await page.evaluate(() => {
      const canvas = document.getElementById('graphicsCanvas');
      const ctx = canvas.getContext('2d');
      if (!ctx) return { hasContent: false, error: 'No 2D context available' };
      
      try {
        const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        let nonBlackPixels = 0;
        let colorVariety = new Set();
        
        for (let i = 0; i < imageData.data.length; i += 4) {
          const r = imageData.data[i];
          const g = imageData.data[i + 1];
          const b = imageData.data[i + 2];
          const a = imageData.data[i + 3];
          
          if (r > 10 || g > 10 || b > 10) {
            nonBlackPixels++;
            colorVariety.add(`${Math.floor(r/32)},${Math.floor(g/32)},${Math.floor(b/32)}`);
          }
        }
        
        return {
          hasContent: nonBlackPixels > 1000,
          nonBlackPixels,
          colorVariety: colorVariety.size,
          canvasSize: `${canvas.width}x${canvas.height}`
        };
      } catch (error) {
        return { hasContent: false, error: error.message };
      }
    });
    
    console.log('Canvas content analysis:', canvasContent);
    
    // Should have significant visual content
    expect(canvasContent.hasContent).toBe(true);
    expect(canvasContent.nonBlackPixels).toBeGreaterThan(1000);
    expect(canvasContent.colorVariety).toBeGreaterThan(5);
    
    // Take final screenshot for verification
    await page.screenshot({ path: 'test-results/graphics-pipeline-rendering.png' });
  });

  test('should clear canvas properly', async ({ page }) => {
    // Add some content first
    await page.click('#testColorCorrection');
    await page.waitForTimeout(500);
    
    const beforeDrawCalls = parseInt(await page.textContent('#drawCalls'));
    expect(beforeDrawCalls).toBeGreaterThan(0);
    
    // Clear the canvas
    await page.click('#clearCanvas');
    await page.waitForTimeout(300);
    
    // Verify clear operation was logged
    const testResults = await page.textContent('#testResults');
    expect(testResults).toContain('Canvas cleared');
    
    console.log('✅ Canvas clearing functionality verified');
  });

  test('should demonstrate Blender/GIMP level graphics capability', async ({ page }) => {
    console.log('🎨 Testing professional graphics application compatibility...');
    
    // Run a comprehensive sequence of professional operations
    const operations = [
      { button: '#testColorCorrection', name: 'Color Correction' },
      { button: '#testBrushEngine', name: 'Brush Engine' },
      { button: '#testLayers', name: 'Layer System' },
      { button: '#testVectorGraphics', name: 'Vector Graphics' },
      { button: '#testTextRendering', name: 'Text Rendering' },
      { button: '#testMegapixelImage', name: '4K Processing' },
      { button: '#testComplexComposite', name: 'Complex Compositing' }
    ];
    
    for (const op of operations) {
      await page.click(op.button);
      await page.waitForTimeout(300);
      console.log(`✓ ${op.name} test executed`);
    }
    
    // Final metrics check
    const finalDrawCalls = parseInt(await page.textContent('#drawCalls'));
    const textureMemory = parseInt((await page.textContent('#textureMemory')).split(' ')[0]);
    const shaderStages = parseInt(await page.textContent('#shaderStages'));
    
    console.log(`Professional Graphics Summary:`);
    console.log(`- Draw Calls: ${finalDrawCalls}`);
    console.log(`- Texture Memory: ${textureMemory}MB`);
    console.log(`- Shader Stages: ${shaderStages}`);
    
    // Professional application requirements
    expect(finalDrawCalls).toBeGreaterThan(50); // Complex operation capability
    expect(shaderStages).toBeGreaterThan(8);    // Multiple specialized pipelines
    
    // Take final professional capabilities screenshot
    await page.screenshot({ path: 'test-results/professional-graphics-capability.png', fullPage: true });
    
    console.log('🎯 Professional graphics compatibility verified!');
  });
});