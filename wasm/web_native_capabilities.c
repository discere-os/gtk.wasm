/*
 * Web-Native Capabilities Detection
 * Runtime detection of browser features for optimal performance
 */

#include <emscripten/emscripten.h>
#include <stdio.h>

typedef struct {
    int has_webgpu;
    int has_wasm_simd;
    int has_shared_array_buffer;
    int has_web_workers;
    int has_fetch_api;
    int chrome_version;
} WebCapabilities;

static WebCapabilities capabilities = {0};

EMSCRIPTEN_KEEPALIVE
void detect_web_capabilities(void) {
    // Detect WebGPU
    EM_ASM({
        Module.webCapabilities = Module.webCapabilities || {};
        Module.webCapabilities.hasWebGPU = !!navigator.gpu;
        Module.webCapabilities.hasWASMSIMD = WebAssembly.validate(new Uint8Array([
            0,97,115,109,1,0,0,0,1,4,1,96,0,0,3,2,1,0,10,9,1,7,0,65,0,253,15,26,11
        ]));
        Module.webCapabilities.hasSharedArrayBuffer = typeof SharedArrayBuffer !== 'undefined';
        Module.webCapabilities.hasWebWorkers = typeof Worker !== 'undefined';
        Module.webCapabilities.hasFetchAPI = typeof fetch !== 'undefined';

        // Detect Chrome version
        const userAgent = navigator.userAgent;
        const chromeMatch = userAgent.match(/Chrome\/(\d+)/);
        Module.webCapabilities.chromeVersion = chromeMatch ? parseInt(chromeMatch[1]) : 0;
    });

    capabilities.has_webgpu = EM_ASM_INT({ return Module.webCapabilities.hasWebGPU ? 1 : 0; });
    capabilities.has_wasm_simd = EM_ASM_INT({ return Module.webCapabilities.hasWASMSIMD ? 1 : 0; });
    capabilities.has_shared_array_buffer = EM_ASM_INT({ return Module.webCapabilities.hasSharedArrayBuffer ? 1 : 0; });
    capabilities.has_web_workers = EM_ASM_INT({ return Module.webCapabilities.hasWebWorkers ? 1 : 0; });
    capabilities.has_fetch_api = EM_ASM_INT({ return Module.webCapabilities.hasFetchAPI ? 1 : 0; });
    capabilities.chrome_version = EM_ASM_INT({ return Module.webCapabilities.chromeVersion; });

    printf("Web Capabilities Detected:\n");
    printf("  WebGPU: %s\n", capabilities.has_webgpu ? "✓" : "✗");
    printf("  WASM SIMD: %s\n", capabilities.has_wasm_simd ? "✓" : "✗");
    printf("  SharedArrayBuffer: %s\n", capabilities.has_shared_array_buffer ? "✓" : "✗");
    printf("  Web Workers: %s\n", capabilities.has_web_workers ? "✓" : "✗");
    printf("  Fetch API: %s\n", capabilities.has_fetch_api ? "✓" : "✗");
    printf("  Chrome Version: %d\n", capabilities.chrome_version);
}

EMSCRIPTEN_KEEPALIVE
WebCapabilities* get_web_capabilities(void) {
    return &capabilities;
}

EMSCRIPTEN_KEEPALIVE
int is_web_native_ready(void) {
    return capabilities.has_webgpu &&
           capabilities.has_wasm_simd &&
           capabilities.chrome_version >= 113;
}
