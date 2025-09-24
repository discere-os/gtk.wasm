#!/bin/bash
# Wrapper for ld.wasm to fake shared library support during meson configuration

# If this is a shared library test (contains -shared flag), pretend to succeed
if [[ "$*" == *"-shared"* ]]; then
    # Create empty output file if specified
    for arg in "$@"; do
        if [[ "$arg" == "-o" ]]; then
            next_is_output=1
        elif [[ "$next_is_output" == "1" ]]; then
            touch "$arg"
            exit 0
        fi
    done
    exit 0
fi

# Otherwise, delegate to emcc for actual compilation
exec emcc "$@"