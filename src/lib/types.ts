/**
 * Type definitions for GTK.wasm
 * Copyright 2025 Superstruct Ltd
 */

export interface GTKConfig {
  /** Enable SIMD optimizations (3-5x speedup) */
  enableSIMD?: boolean;
  /** Enable WebGPU acceleration (10x+ speedup) */
  enableWebGPU?: boolean;
  /** Enable threading support (10x speedup) */
  enableThreading?: boolean;
  /** Initial memory size in bytes */
  initialMemory?: number;
  /** Maximum memory size in bytes */
  maximumMemory?: number;
}

export interface PerformanceMetrics {
  /** SIMD speedup multiplier (target: 3-5x) */
  simdSpeedup: number;
  /** WebGPU speedup multiplier (target: 10x+) */
  webgpuSpeedup: number;
  /** Workers speedup multiplier (target: 10x) */
  workersSpeedup: number;
  /** Crypto speedup multiplier (target: 5-15x) */
  cryptoSpeedup: number;
}

export interface WebCapabilities {
  /** WASM SIMD support (required for Chrome 113+) */
  has_wasm_simd: boolean;
  /** WebGPU support (required for GPU rendering) */
  has_webgpu: boolean;
  /** SharedArrayBuffer support (required for threading) */
  has_shared_array_buffer: boolean;
  /** Web Crypto API support (5-15x crypto speedup) */
  has_web_crypto: boolean;
  /** Origin Private File System (3-4x storage speedup) */
  has_opfs: boolean;
  /** Web Workers support (10x threading speedup) */
  has_workers: boolean;
  /** Fetch API support (3-5x network speedup) */
  has_fetch_api: boolean;
  /** Chrome version number */
  chrome_version: number;
}

export interface GTKModule {
  ccall: (funcName: string, returnType: string, argTypes: string[], args: any[]) => any;
  cwrap: (funcName: string, returnType: string, argTypes: string[]) => Function;
  FS: any;
  HEAPU8: Uint8Array;
  _malloc: (size: number) => number;
  _free: (ptr: number) => void;
  getValue: (ptr: number, type: string) => number;
  setValue: (ptr: number, value: number, type: string) => void;
  UTF8ToString: (ptr: number) => string;
  stringToUTF8: (str: string, ptr: number, maxLength: number) => void;
}

export class GTKError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'GTKError';
  }
}

export enum StorageTier {
  MEMORY = 0,     // Fastest, temporary
  OPFS = 1,       // Fast, persistent (3-4x vs IDBFS)
  CACHE = 2,      // Medium, browser cache
  IDBFS = 3,      // Slowest, IndexedDB (legacy)
}

export interface MemoryInfo {
  used_mb: number;
  total_mb: number;
  limit_mb: number;
  pressure_level: number; // 0=normal, 1=medium, 2=high, 3=critical
}

export interface RenderingInfo {
  backend: 'webgpu' | 'cairo' | 'software';
  has_gpu_acceleration: boolean;
  frame_time_ms: number;
}

// SIMD string functions
export interface SIMDStringOps {
  strlen: (str: string) => number;
  memcmp: (s1: Uint8Array, s2: Uint8Array) => number;
  memcpy: (dest: Uint8Array, src: Uint8Array) => void;
  strstr: (haystack: string, needle: string) => number;
}

// Crypto functions
export interface CryptoOps {
  sha256: (data: Uint8Array) => Promise<Uint8Array>;
  randomBytes: (len: number) => Uint8Array;
  aesEncrypt: (key: Uint8Array, iv: Uint8Array, data: Uint8Array) => Promise<Uint8Array>;
}

// Threading functions
export interface ThreadingOps {
  hasSharedArrayBuffer: () => boolean;
  hasWorkers: () => boolean;
  getOptimalThreadCount: () => number;
  initThreadPool: () => void;
  getThreadPoolSize: () => number;
}

// Networking functions
export interface NetworkingOps {
  fetchGet: (url: string) => Promise<Uint8Array>;
  fetchPost: (url: string, body: Uint8Array) => Promise<Uint8Array>;
  websocketAvailable: () => boolean;
}

// Filesystem functions
export interface FilesystemOps {
  hasOPFS: () => boolean;
  readOPFS: (path: string) => Promise<Uint8Array>;
  writeOPFS: (path: string, data: Uint8Array) => Promise<boolean>;
  storageEstimate: () => Promise<{ usage: number; quota: number }>;
}

// Memory functions
export interface MemoryOps {
  getPressureLevel: () => number;
  logMemoryInfo: () => void;
  hintGC: () => void;
  mallocTracked: (size: number, name: string) => number;
  freeTracked: (ptr: number) => void;
}
