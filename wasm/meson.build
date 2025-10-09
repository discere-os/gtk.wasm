# GTK WASM Module Targets
# Copyright 2025 Superstruct Ltd

# Base compiler flags
gtk_wasm_cargs = ['-DGTK_WASM=1']

# SIMD support
if get_option('wasm_simd')
  gtk_wasm_cargs += ['-msimd128']
endif

# Threading support
if get_option('wasm_threading')
  gtk_wasm_cargs += ['-pthread']
endif

# Build type configuration
build_type = get_option('wasm_build_type')

# Base link args for MAIN module
gtk_main_link_args = [
  '-sWASM=1',
  '-sMODULARIZE=1',
  '-sEXPORT_ES6=1',
  '-sEXPORT_NAME=GtkModule',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sENVIRONMENT=web,webview,worker',
  '-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString","stringToUTF8","getValue","setValue","FS"]',
]

# Optimization
if get_option('wasm_optimize') == 'size'
  gtk_main_link_args += ['-Os']
elif get_option('wasm_optimize') == 'speed'
  gtk_main_link_args += ['-O3', '-flto']
else
  gtk_main_link_args += ['-O3']
endif

# Standard build: Add threading
if build_type == 'standard' or build_type == 'webgpu'
  if get_option('wasm_threading')
    gtk_main_link_args += [
      '-pthread',
      '-sPTHREAD_POOL_SIZE=4',
      '-sINITIAL_MEMORY=134217728',  # 128MB
      '-sMAXIMUM_MEMORY=1073741824',  # 1GB
    ]
  endif
endif

# WebGPU build: Add WebGPU port and ASYNCIFY
if build_type == 'webgpu'
  if get_option('wasm_webgpu')
    gtk_main_link_args += [
      '--use-port=emdawnwebgpu',
      '-sASYNCIFY=1',
      '-sASYNCIFY_STACK_SIZE=32768',
    ]
  endif

  # Add more runtime methods for WebGPU demos
  gtk_main_link_args += [
    '-sFETCH=1',
    '-sWASM_BIGINT=1',
  ]
endif

# Minimal build: Reduce size
if build_type == 'minimal'
  gtk_main_link_args += [
    '-sNO_FILESYSTEM=1',
    '-sASSERTIONS=0',
  ]
else
  gtk_main_link_args += [
    '-sFORCE_FILESYSTEM=1',
  ]
endif

# MAIN_MODULE executable
executable(
  'gtk-main',
  files('gtk_wasm_module.c'),
  dependencies: [libgtk_dep],
  c_args: gtk_wasm_cargs,
  link_args: gtk_main_link_args,
  install: true,
  install_dir: get_option('bindir'),
)

# SIDE_MODULE for dynamic linking
gtk_side_link_args = [
  '-sSIDE_MODULE=2',
  '-fPIC',
  '-sEXPORTED_FUNCTIONS=["_gtk_wasm_version"]',
]

# Optimization for SIDE module
if get_option('wasm_optimize') == 'size'
  gtk_side_link_args += ['-Os']
else
  gtk_side_link_args += ['-O3', '-flto']
endif

shared_module(
  'gtk-side',
  files('gtk_wasm_side.c'),
  dependencies: [libgtk_dep],
  name_prefix: '',
  c_args: gtk_wasm_cargs,
  link_args: gtk_side_link_args,
  install: true,
  install_dir: get_option('libdir'),
)
