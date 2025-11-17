/*
 * Web-Native Filesystem Operations
 * 3-4x performance boost using OPFS vs IDBFS
 * Copyright 2025 Superstruct Ltd
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    STORAGE_MEMORY,     // Fastest, temporary
    STORAGE_OPFS,       // Fast, persistent (3-4x vs IDBFS)
    STORAGE_CACHE,      // Medium, browser cache
    STORAGE_IDBFS       // Slowest, IndexedDB (legacy)
} StorageTier;

// Check if OPFS is available
EMSCRIPTEN_KEEPALIVE
int web_has_opfs(void) {
    return EM_ASM_INT({
        return typeof navigator.storage !== 'undefined' &&
               typeof navigator.storage.getDirectory !== 'undefined' ? 1 : 0;
    });
}

// Read file from OPFS (3-4x faster than IDBFS)
EM_JS(void, web_storage_read_opfs_js, (
    const char* path,
    void (*callback)(const uint8_t*, size_t, void*),
    void* user_data
), {
    const pathStr = UTF8ToString(path);

    if (typeof navigator.storage === 'undefined' ||
        typeof navigator.storage.getDirectory === 'undefined') {
        console.warn('OPFS not available');
        dynCall('viii', callback, [0, 0, user_data]);
        return;
    }

    navigator.storage.getDirectory()
        .then(root => root.getFileHandle(pathStr, { create: false }))
        .then(handle => handle.getFile())
        .then(file => file.arrayBuffer())
        .then(buffer => {
            const data = new Uint8Array(buffer);
            const ptr = _malloc(data.length);
            HEAPU8.set(data, ptr);

            dynCall('viii', callback, [ptr, data.length, user_data]);
            _free(ptr);
        })
        .catch(err => {
            console.error('OPFS read error:', err);
            dynCall('viii', callback, [0, 0, user_data]);
        });
});

typedef void (*read_callback_t)(const uint8_t* data, size_t len, void* user_data);

EMSCRIPTEN_KEEPALIVE
void web_storage_read_opfs(const char* path, read_callback_t callback, void* user_data) {
    if (web_has_opfs()) {
        web_storage_read_opfs_js(path, callback, user_data);
    } else {
        callback(NULL, 0, user_data);
    }
}

// Write file to OPFS
EM_JS(void, web_storage_write_opfs_js, (
    const char* path,
    const uint8_t* data,
    size_t len,
    void (*callback)(int, void*),
    void* user_data
), {
    const pathStr = UTF8ToString(path);
    const fileData = HEAPU8.slice(data, data + len);

    if (typeof navigator.storage === 'undefined' ||
        typeof navigator.storage.getDirectory === 'undefined') {
        console.warn('OPFS not available');
        dynCall('vii', callback, [0, user_data]);
        return;
    }

    navigator.storage.getDirectory()
        .then(root => root.getFileHandle(pathStr, { create: true }))
        .then(handle => handle.createWritable())
        .then(writable => {
            return writable.write(fileData).then(() => writable.close());
        })
        .then(() => {
            dynCall('vii', callback, [1, user_data]);
        })
        .catch(err => {
            console.error('OPFS write error:', err);
            dynCall('vii', callback, [0, user_data]);
        });
});

typedef void (*write_callback_t)(int success, void* user_data);

EMSCRIPTEN_KEEPALIVE
void web_storage_write_opfs(const char* path, const uint8_t* data, size_t len,
                             write_callback_t callback, void* user_data) {
    if (web_has_opfs()) {
        web_storage_write_opfs_js(path, data, len, callback, user_data);
    } else {
        callback(0, user_data);
    }
}

// Get storage estimate
EMSCRIPTEN_KEEPALIVE
void web_storage_estimate(void) {
    EM_ASM({
        if (typeof navigator.storage !== 'undefined' &&
            typeof navigator.storage.estimate !== 'undefined') {
            navigator.storage.estimate().then(estimate => {
                const used = (estimate.usage / 1024 / 1024).toFixed(2);
                const total = (estimate.quota / 1024 / 1024).toFixed(2);
                console.log(`Storage: ${used} MB / ${total} MB`);
            });
        }
    });
}
