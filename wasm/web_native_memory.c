/*
 * Web-Native Memory Management
 * Integration with browser GC and memory pressure APIs
 * Copyright 2025 Superstruct Ltd
 */

#include <emscripten.h>
#include <stdlib.h>

// Register WeakRef for automatic GC integration
EM_JS(void, register_weak_ref_js, (void* ptr, const char* name), {
    if (typeof WeakRef === 'undefined') {
        return;
    }

    const nameStr = UTF8ToString(name);
    const ref = new WeakRef({ ptr: ptr, name: nameStr });

    // Store in global registry for cleanup
    if (!Module.weakRefs) {
        Module.weakRefs = new Map();
    }
    Module.weakRefs.set(ptr, ref);
});

EMSCRIPTEN_KEEPALIVE
void register_weak_ref(void* ptr, const char* name) {
    int has_weak_ref = EM_ASM_INT({
        return typeof WeakRef !== 'undefined' ? 1 : 0;
    });

    if (has_weak_ref) {
        register_weak_ref_js(ptr, name);
    }
}

// Memory pressure monitoring
EMSCRIPTEN_KEEPALIVE
int web_memory_pressure_level(void) {
    return EM_ASM_INT({
        if (typeof performance === 'undefined' ||
            typeof performance.memory === 'undefined') {
            return 0; // Unknown
        }

        const memory = performance.memory;
        const usedPercent = (memory.usedJSHeapSize / memory.jsHeapSizeLimit) * 100;

        if (usedPercent > 90) return 3; // Critical
        if (usedPercent > 75) return 2; // High
        if (usedPercent > 50) return 1; // Medium
        return 0; // Normal
    });
}

// Get memory info
EMSCRIPTEN_KEEPALIVE
void web_log_memory_info(void) {
    EM_ASM({
        if (typeof performance !== 'undefined' &&
            typeof performance.memory !== 'undefined') {
            const mem = performance.memory;
            console.log('Memory Info:');
            console.log('  Used:', (mem.usedJSHeapSize / 1024 / 1024).toFixed(2), 'MB');
            console.log('  Total:', (mem.totalJSHeapSize / 1024 / 1024).toFixed(2), 'MB');
            console.log('  Limit:', (mem.jsHeapSizeLimit / 1024 / 1024).toFixed(2), 'MB');
        } else {
            console.log('Memory API not available');
        }
    });
}

// Request garbage collection hint
EMSCRIPTEN_KEEPALIVE
void web_hint_gc(void) {
    EM_ASM({
        // This is just a hint to the browser
        // Actual GC is at browser's discretion
        if (typeof gc !== 'undefined') {
            gc(); // V8-specific
        }
    });
}

// Allocate with tracking
EMSCRIPTEN_KEEPALIVE
void* web_malloc_tracked(size_t size, const char* name) {
    void* ptr = malloc(size);
    if (ptr) {
        register_weak_ref(ptr, name);
    }
    return ptr;
}

// Free with cleanup
EMSCRIPTEN_KEEPALIVE
void web_free_tracked(void* ptr) {
    if (!ptr) return;

    EM_ASM({
        if (Module.weakRefs) {
            Module.weakRefs.delete($0);
        }
    }, ptr);

    free(ptr);
}
