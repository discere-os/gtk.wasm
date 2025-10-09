# GSK WASM Compare Tests
# Automated testing of GSK render nodes against reference PNGs using WASM

# Build GSK node renderer WASM module
gsk_node_renderer_sources = [
  'gsk-node-renderer.c',
]

gsk_node_renderer_c_args = [
  '-O3',
  '-flto',
  '-msimd128',
]

gsk_node_renderer_link_args = [
  '-sWASM=1',
  '-sMODULARIZE=1',
  '-sEXPORT_ES6=1',
  '-sEXPORT_NAME="GSKNodeRenderer"',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sALLOW_TABLE_GROWTH=1',
  '-sINITIAL_MEMORY=134217728',
  '-sMAXIMUM_MEMORY=2147483648',
  '-sENVIRONMENT=web,webview,worker,node',
  '-sEXPORTED_FUNCTIONS=["_main","_init_renderer","_render_node_file","_get_render_data","_get_render_width","_get_render_height","_get_render_stride","_cleanup_renderer"]',
  '-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString","stringToUTF8","_malloc","_free","HEAPU8"]',
  '-sINVOKE_RUN=0',
  '-sERROR_ON_UNDEFINED_SYMBOLS=0',
  '-sWARN_ON_UNDEFINED_SYMBOLS=0',
  '-Wl,--export-dynamic',
]

# Note: This executable is built for WASM, so it produces .js and .wasm files
gsk_node_renderer_wasm = executable(
  'gsk-node-renderer',
  gsk_node_renderer_sources,
  dependencies: libgtk_static_dep,
  c_args: gsk_node_renderer_c_args + common_cflags + ['-DGTK_COMPILATION'],
  link_args: gsk_node_renderer_link_args,
  install: false,
)

# Find Deno executable
deno = find_program('deno', required: false)

if deno.found()
  # Run WASM compare tests with Deno
  test(
    'gsk-compare-wasm',
    deno,
    args: [
      'run',
      '--allow-read',
      '--allow-write',
      '--no-check',
      meson.current_source_dir() / 'run-compare-tests.ts',
      '--dir', meson.current_source_dir() / '../compare',
      '--output', meson.current_build_dir() / 'test-output',
    ],
    env: {
      'WASM_MODULE_DIR': meson.current_build_dir(),
    },
    timeout: 600, # 10 minutes for all tests
    suite: ['gsk', 'gsk-wasm', 'gsk-compare-wasm'],
    is_parallel: false, # Run sequentially to avoid resource contention
  )

  # Add individual test variations as separate tests for better CI reporting
  test_variations = {
    'plain': '--plain',
    # TODO: Add more variations as they are implemented
    # 'flip': '--flip',
    # 'rotate': '--rotate',
    # 'repeat': '--repeat',
    # 'mask': '--mask',
    # 'replay': '--replay',
    # 'clip': '--clip',
    # 'colorflip': '--colorflip',
  }

  # Optional: Run quick smoke test with a few known-good tests
  smoke_tests = [
    'blend-modes',
    'color-matrix-identity',
    'empty-color',
    'border-one-rounded',
    'clip-contained',
  ]

  foreach testname : smoke_tests
    test(
      'gsk-wasm-smoke-' + testname,
      deno,
      args: [
        'run',
        '--allow-read',
        '--allow-write',
        '--no-check',
        meson.current_source_dir() / 'run-compare-tests.ts',
        '--dir', meson.current_source_dir() / '../compare',
        '--output', meson.current_build_dir() / 'test-output',
        '--test', testname,
      ],
      env: {
        'WASM_MODULE_DIR': meson.current_build_dir(),
      },
      timeout: 60, # 1 minute per smoke test
      suite: ['gsk', 'gsk-wasm', 'gsk-wasm-smoke'],
      is_parallel: true,
    )
  endforeach
else
  message('Deno not found, skipping WASM compare tests')
endif
