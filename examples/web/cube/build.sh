#!/bin/bash
set -e

echo "Building WebGPU Cube Example for Web..."

mkdir -p web_build

# Compile the GfxWrapper library sources for WebGPU
GFX_SOURCES=(
    "../../../gfx/src/api/GfxDispatcher.cpp"
    "../../../gfx/src/backend/BackendFactory.cpp"
    "../../../gfx/src/backend/webgpu/WebGPUBackend.cpp"
)

# Build with emdawnwebgpu port
emcc cube_example.c ${GFX_SOURCES[@]} \
    -o web_build/index.html \
    -I../../../gfx/include \
    -I../../../gfx/src \
    -DGFX_ENABLE_WEBGPU \
    --use-port=emdawnwebgpu \
    -sUSE_GLFW=3 \
    -sASYNCIFY=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sASSERTIONS=2 \
    -sSTACK_OVERFLOW_CHECK=2 \
    -sSAFE_HEAP=1 \
    -g \
    -O0

echo ""
echo "Build complete!"
echo "Run: ./serve.sh"
echo "Then open: http://localhost:8765/"
