/*
 * Web-Native Main Loop
 * RequestAnimationFrame integration for UI libraries
 * Copyright 2025 Superstruct Ltd
 */

#include <emscripten.h>
#include <emscripten/html5.h>

typedef void (*loop_callback_t)(void* user_data);

static loop_callback_t g_loop_callback = NULL;
static void* g_loop_user_data = NULL;
static int g_loop_running = 0;

// Internal loop function
static void main_loop_tick(void) {
    if (g_loop_callback && g_loop_running) {
        g_loop_callback(g_loop_user_data);
    }
}

// Setup RequestAnimationFrame loop
EM_JS(void, setup_raf_loop_js, (void (*callback)(void)), {
    if (Module.rafLoopId) {
        cancelAnimationFrame(Module.rafLoopId);
    }

    function loop() {
        dynCall('v', callback, []);
        if (Module.rafLoopRunning) {
            Module.rafLoopId = requestAnimationFrame(loop);
        }
    }

    Module.rafLoopRunning = true;
    Module.rafLoopId = requestAnimationFrame(loop);
});

EMSCRIPTEN_KEEPALIVE
void web_setup_raf_loop(loop_callback_t callback, void* user_data) {
    g_loop_callback = callback;
    g_loop_user_data = user_data;
    g_loop_running = 1;

    setup_raf_loop_js(main_loop_tick);
}

// Stop the loop
EMSCRIPTEN_KEEPALIVE
void web_stop_raf_loop(void) {
    g_loop_running = 0;

    EM_ASM({
        if (Module.rafLoopId) {
            cancelAnimationFrame(Module.rafLoopId);
            Module.rafLoopId = null;
            Module.rafLoopRunning = false;
        }
    });
}

// Check if loop is running
EMSCRIPTEN_KEEPALIVE
int web_is_loop_running(void) {
    return g_loop_running;
}

// Idle callback (runs when browser is idle)
EM_JS(void, request_idle_callback_js, (void (*callback)(void)), {
    if (typeof requestIdleCallback !== 'undefined') {
        requestIdleCallback(() => {
            dynCall('v', callback, []);
        });
    } else {
        // Fallback to setTimeout
        setTimeout(() => {
            dynCall('v', callback, []);
        }, 0);
    }
});

typedef void (*idle_callback_t)(void);

EMSCRIPTEN_KEEPALIVE
void web_request_idle_callback(idle_callback_t callback) {
    request_idle_callback_js(callback);
}

// Get frame timing information
EMSCRIPTEN_KEEPALIVE
double web_get_frame_time(void) {
    return EM_ASM_DOUBLE({
        return performance.now();
    });
}

// Request next frame
EMSCRIPTEN_KEEPALIVE
void web_request_animation_frame(void (*callback)(double time)) {
    EM_ASM({
        requestAnimationFrame((time) => {
            dynCall('vd', $0, [time]);
        });
    }, callback);
}
