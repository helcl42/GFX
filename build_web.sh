#!/bin/bash
set -e

echo "=== Building GfxWrapper for WebAssembly ==="
echo ""

# Check if emcmake is available
if ! command -v emcmake &> /dev/null; then
    echo "Error: emcmake not found. Please install Emscripten SDK."
    echo "Visit: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create build directory
BUILD_DIR="build_web"
mkdir -p "$BUILD_DIR"

echo "Configuring with Emscripten..."
cd "$BUILD_DIR"

# Configure with emcmake
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_WEBGPU_BACKEND=ON \
    -DBUILD_VULKAN_BACKEND=OFF

echo ""
echo "Building..."
emmake make -j$(nproc)

echo ""
echo "=== Build Complete! ==="
echo ""
echo "Web files are in: $BUILD_DIR/web/"
echo ""
echo "To run:"
echo "  cd $BUILD_DIR/web"
echo "  python3 -m http.server 8000"
echo "  Then open: http://localhost:8000/cube_example_web.html"
echo ""
