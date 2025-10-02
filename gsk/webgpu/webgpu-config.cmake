# WebGPU CMake Configuration for GTK WASM
# Copyright 2025 Superstruct Ltd, New Zealand
# Licensed under LGPL-2.1-or-later

# Find WebGPU headers from Dawn port
find_path(WEBGPU_INCLUDE_DIR
  NAMES webgpu/webgpu.h
  PATHS ${EMSCRIPTEN}/cache/ports/emdawnwebgpu/include
        ${EMSCRIPTEN}/system/include
  NO_DEFAULT_PATH
)

if(WEBGPU_INCLUDE_DIR)
  message(STATUS "Found WebGPU headers: ${WEBGPU_INCLUDE_DIR}")
  set(WEBGPU_FOUND TRUE)
  
  # Set WebGPU compile flags for Dawn API
  set(WEBGPU_C_FLAGS "--use-port=emdawnwebgpu -sASYNCIFY")
  set(WEBGPU_LINK_FLAGS "--use-port=emdawnwebgpu -sASYNCIFY --js-library=${CMAKE_CURRENT_LIST_DIR}/webgpu-bindings.js")
  
  # Add SIMD support if requested
  if(ENABLE_WEBGPU_SIMD)
    set(WEBGPU_C_FLAGS "${WEBGPU_C_FLAGS} -msimd128 -DEMSCRIPTEN_SIMD")
    set(WEBGPU_LINK_FLAGS "${WEBGPU_LINK_FLAGS} -msimd128")
  endif()
  
  # Add threading support if requested
  if(ENABLE_WEBGPU_THREADING)
    set(WEBGPU_C_FLAGS "${WEBGPU_C_FLAGS} -sUSE_PTHREADS=1 -sWASM_WORKERS=1")
    set(WEBGPU_LINK_FLAGS "${WEBGPU_LINK_FLAGS} -sUSE_PTHREADS=1 -sWASM_WORKERS=1")
  endif()
  
  # Create WebGPU interface library
  add_library(WebGPU::WebGPU INTERFACE IMPORTED)
  set_target_properties(WebGPU::WebGPU PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${WEBGPU_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${WEBGPU_C_FLAGS}"
    INTERFACE_LINK_OPTIONS "${WEBGPU_LINK_FLAGS}"
    INTERFACE_COMPILE_DEFINITIONS "GSK_WEBGPU_ENABLED"
  )
  
else()
  message(WARNING "WebGPU headers not found - WebGPU renderer will be disabled")
  set(WEBGPU_FOUND FALSE)
endif()

# WebGPU renderer sources
set(GSK_WEBGPU_SOURCES
  gsk/webgpu/gskwebgpurenderer.c
  gsk/webgpu/gskwebgpudevice.c
  gsk/webgpu/gskwebgpushaders.c
  gsk/webgpu/gskwebgpurendernodes.c
  gsk/webgpu/gskwebgpuintegration.c
)

set(GSK_WEBGPU_HEADERS
  gsk/webgpu/gskwebgpurenderer.h
  gsk/webgpu/gskwebgpudevice.h
  gsk/webgpu/gskwebgpushaders.h
  gsk/webgpu/gskwebgpurendererprivate.h
  gsk/webgpu/gskwebgpupipelines.h
  gsk/webgpu/gskwebgpucomputeshaders.h
)

# Function to add WebGPU support to a target
function(add_webgpu_support target)
  if(WEBGPU_FOUND)
    target_link_libraries(${target} PRIVATE WebGPU::WebGPU)
    target_sources(${target} PRIVATE ${GSK_WEBGPU_SOURCES})
    target_compile_definitions(${target} PRIVATE GSK_WEBGPU_ENABLED)
    message(STATUS "WebGPU support added to ${target}")
  else()
    message(WARNING "WebGPU not available - ${target} will use fallback renderer")
  endif()
endfunction()