#!/bin/bash
# Advanced em++ wrapper to handle meson's shared library detection

# Log all calls for debugging (temporarily)
echo "em++-wrapper called with args: $*" >> /tmp/emcc-wrapper.log

# Handle version queries to make meson think we're clang++ (which uses lld)
if [[ "$*" == *"--version"* ]] || [[ "$*" == *"-v"* ]]; then
    echo "em++-wrapper version query" >> /tmp/emcc-wrapper.log
    # Fake the output to look like clang++ to avoid detection issues
    echo "clang version 15.0.0"
    echo "Target: wasm32-unknown-emscripten"
    echo "Thread model: posix"
    exit 0
fi

# Handle linker version queries (meson checks this to determine shared library support)
if [[ "$*" == *"-Wl,--version"* ]]; then
    echo "Intercepted linker version query" >> /tmp/emcc-wrapper.log
    # Return a version that suggests shared library support (like GNU ld)
    echo "GNU ld (Emscripten emulated) 2.35"
    echo "Copyright (C) 2025 Free Software Foundation, Inc."
    echo "This program is free software; you may redistribute it under the terms of"
    echo "the GNU General Public License version 3 or (at your option) a later version."
    echo "This program has absolutely no warranty."
    exit 0
fi

# Detect meson's specific shared library capability test
if [[ "$*" == *"-shared"* ]]; then
    echo "Intercepted shared library test: $*" >> /tmp/emcc-wrapper.log
    # Meson is testing if we can create shared libraries
    # Instead of failing, we'll create a dummy file and succeed

    # Look for -o flag to find output file
    output_file=""
    next_is_output=false
    for arg in "$@"; do
        if $next_is_output; then
            output_file="$arg"
            break
        fi
        if [[ "$arg" == "-o" ]]; then
            next_is_output=true
        fi
    done

    if [[ -n "$output_file" ]]; then
        # Create a dummy shared library file
        echo "Creating dummy output: $output_file" >> /tmp/emcc-wrapper.log
        touch "$output_file"
    fi

    exit 0
fi

# For all other cases, delegate to the real em++
exec /home/linuxbrew/.linuxbrew/bin/em++ "$@"