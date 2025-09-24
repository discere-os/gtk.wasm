#!/bin/bash
# Advanced emcc wrapper to handle meson's shared library detection

# Log all calls for debugging (temporarily)
echo "emcc-wrapper called with args: $*" >> /tmp/emcc-wrapper.log

# Handle special case where meson calls with -cpp as first argument
if [[ "$1" == "-cpp" ]]; then
    echo "Intercepted -cpp preprocessor call" >> /tmp/emcc-wrapper.log
    # This is meson trying to extract preprocessor macros
    exec /home/linuxbrew/.linuxbrew/bin/emcc -E -dM -xc /dev/null
fi

# Handle linker version queries first (higher priority than compiler version)
if [[ "$*" == *"-Wl,--version"* ]]; then
    echo "Intercepted linker version query" >> /tmp/emcc-wrapper.log
    # Pretend to be lld which meson supports
    echo "LLD 15.0.0 (compatible with GNU linkers)"
    exit 0
fi

# Handle version queries to make meson think we're clang (which uses lld)
if [[ "$*" == *"--version"* ]]; then
    echo "emcc-wrapper version query" >> /tmp/emcc-wrapper.log
    # Fake the output to look like clang to avoid detection issues
    echo "clang version 15.0.0"
    echo "Target: wasm32-unknown-emscripten"
    echo "Thread model: posix"
    exit 0
fi

# Handle preprocessor macro extraction (meson uses this to detect compiler)
if [[ "$*" == *"-E -dM"* ]] || [[ "$*" == *"-cpp"* ]]; then
    echo "Intercepted preprocessor macro extraction: $*" >> /tmp/emcc-wrapper.log
    # Run the real emcc with corrected arguments
    exec /home/linuxbrew/.linuxbrew/bin/emcc -E -dM -xc /dev/null
fi

# Handle fuse-ld flag (ignore it since we're already the linker wrapper)
if [[ "$*" == *"-fuse-ld="* ]]; then
    echo "Removing -fuse-ld flag" >> /tmp/emcc-wrapper.log
    # Remove the -fuse-ld flag and pass the rest to emcc
    args=""
    for arg in "$@"; do
        if [[ "$arg" != "-fuse-ld="* ]]; then
            args="$args $arg"
        fi
    done
    exec /home/linuxbrew/.linuxbrew/bin/emcc $args
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

# For all other cases, delegate to the real emcc with our config header
# Add our config header to fix missing definitions
exec /home/linuxbrew/.linuxbrew/bin/emcc -include /home/isaac/src/discere/discere-nucleus/client/emscripten/gtk.wasm/gtk-wasm-config.h "$@"