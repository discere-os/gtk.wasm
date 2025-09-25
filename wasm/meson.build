gtk_wasm_cargs = ['-msimd128', '-DGTK_WASM=1']

gtk_main_link_args = [
  '-sWASM=1',
  '-sMODULARIZE=1',
  '-sEXPORT_ES6=1',
  '-sEXPORT_NAME=GtkModule',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sENVIRONMENT=web,webview,worker',
  '-sNO_FILESYSTEM=1',
  '-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString"]',
  '-O3',
  '-flto',
]

executable(
  'gtk-main',
  files('gtk_wasm_module.c'),
  dependencies: [libgtk_dep],
  c_args: gtk_wasm_cargs,
  link_args: gtk_main_link_args,
  install: true,
  install_dir: get_option('bindir'),
)

gtk_side_link_args = [
  '-sSIDE_MODULE=2',
  '-fPIC',
  '-O3',
  '-flto',
  '-sEXPORTED_FUNCTIONS=["_gtk_wasm_version"]',
]

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

