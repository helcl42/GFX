# Building for WebAssembly

## Prerequisites

1. Install Emscripten SDK:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

## Building

Run the build script:
```bash
./build_web.sh
```

This will:
- Create a `build_web/` directory
- Configure CMake with Emscripten toolchain
- Build the GfxWrapper library for WebAssembly
- Build the web cube example

## Running

After building, start a local web server:
```bash
cd build_web/web
python3 -m http.server 8000
```

Then open your browser to: http://localhost:8000/cube_example_web.html

## How it works

The CMake configuration automatically detects when building with Emscripten and:
- Disables Vulkan backend (not available on web)
- Enables WebGPU backend with Dawn
- Links with Emscripten's GLFW and WebGPU port
- Generates HTML/JS/WASM output files
- Uses the GfxWrapper library just like native builds!

## Key differences from manual build.sh

The CMake approach:
- ✅ Builds a proper library (libgfx.a) that can be reused
- ✅ Only recompiles changed files (faster rebuilds)
- ✅ Unified build system for native and web
- ✅ Easier to maintain and extend
- ✅ Better dependency management
