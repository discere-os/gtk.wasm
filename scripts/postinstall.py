#!/usr/bin/env python3
import os, sys, shutil

def main():
    prefix = os.environ.get('MESON_INSTALL_DESTDIR_PREFIX') or (sys.argv[1] if len(sys.argv) > 1 else None)
    if not prefix:
        prefix = os.getcwd()
    wasm_dir = os.path.join(prefix, 'wasm')
    if not os.path.isdir(wasm_dir):
        return 0
    target = os.path.join(wasm_dir, 'gtk-side.wasm')
    if os.path.exists(target):
        return 0
    for f in os.listdir(wasm_dir):
        if f.endswith('.wasm'):
            shutil.copy2(os.path.join(wasm_dir, f), target)
            break
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

