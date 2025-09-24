# WebGPU renderer for GTK WASM
# Copyright 2025 Superstruct Ltd, New Zealand
# Licensed under LGPL-2.1-or-later

# Include pipeline management in build
webgpu_sources = [
  'gskwebgpurenderer.c',
  'gskwebgpudevice.c', 
  'gskwebgpushaders.c',
  'gskwebgpurendernodes.c',
  'gskwebgpuintegration.c',
  'gskwebgpupipelines.c',
]

webgpu_headers = [
  'gskwebgpurenderer.h',
  'gskwebgpudevice.h',
  'gskwebgpushaders.h',
  'gskwebgpurendererprivate.h',
  'gskwebgpupipelines.h',
  'gskwebgpucomputeshaders.h',
]

webgpu_c_args = [
  '-DGSK_COMPILATION',
  '-DG_LOG_DOMAIN="Gsk"',
  '-DGSK_WEBGPU_ENABLED',
]

# Add Emscripten WebGPU support with comprehensive shader preloading
if get_option('webgpu-renderer').enabled()
  webgpu_c_args += [
    '-sUSE_WEBGPU=1',
    '-sASYNCIFY',
    '--preload-file=@0@/shaders@/shaders'.format(meson.current_source_dir()),
    '-sALLOW_MEMORY_GROWTH=1',
    '-sMAXIMUM_MEMORY=2GB',
    '-sFILESYSTEM=1'
  ]
  
  # Add SIMD support if requested
  if get_option('simd_support')
    webgpu_c_args += ['-msimd128', '-DEMSCRIPTEN_SIMD']
  endif
  
  # Add threading support if requested
  if get_option('threading_support')
    webgpu_c_args += ['-sUSE_PTHREADS=1', '-sWASM_WORKERS=1']
  endif
endif

# Create WebGPU renderer library
libgsk_webgpu = static_library(
  'gsk-webgpu',
  webgpu_sources,
  include_directories: [confinc, gskinc],
  dependencies: [glib_dep, gobject_dep, graphene_dep],
  c_args: webgpu_c_args,
  install: false,
)

# Export for parent build
gsk_webgpu_dep = declare_dependency(
  link_with: libgsk_webgpu,
  include_directories: include_directories('.'),
  dependencies: [glib_dep, gobject_dep],
)