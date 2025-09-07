/**
 * Global setup for GTK.wasm WebGPU testing
 * Ensures test environment is properly configured
 */

import { execSync } from 'child_process';
import { existsSync } from 'fs';
import path from 'path';

async function globalSetup() {
  console.log('🚀 Setting up GTK.wasm WebGPU test environment...');

  // Check if required demo files exist
  const demoPath = path.join(process.cwd(), 'examples', 'webgpu-hybrid-demo.html');
  if (!existsSync(demoPath)) {
    console.warn('⚠️  WebGPU hybrid demo file not found at:', demoPath);
    console.warn('   Tests may fail if demo files are missing');
  } else {
    console.log('✅ WebGPU hybrid demo file found');
  }

  // Check if GTK WASM files exist
  const wasmPath = path.join(process.cwd(), 'examples', 'dist');
  if (!existsSync(wasmPath)) {
    console.warn('⚠️  GTK WASM dist directory not found at:', wasmPath);
    console.warn('   Run `make all` in examples/ to build WASM files');
  } else {
    console.log('✅ GTK WASM dist directory found');
  }

  // Verify Emscripten environment (if needed)
  try {
    execSync('emcc --version', { stdio: 'pipe' });
    console.log('✅ Emscripten environment available');
  } catch (error) {
    console.warn('⚠️  Emscripten not found - build operations may fail');
  }

  // Log WebGPU testing notes
  console.log('📋 WebGPU Testing Notes:');
  console.log('   • WebGPU support varies by browser and system');
  console.log('   • Tests will fall back to Canvas 2D if WebGPU unavailable'); 
  console.log('   • WSL2 environments may not support WebGPU hardware acceleration');
  console.log('   • For best results, run on native Windows/macOS/Linux with GPU');

  console.log('🎯 Global setup completed');
}

export default globalSetup;