# Building for Web (Emscripten/WebAssembly) with CMake

GfxWrapper can be built for web using Emscripten and CMake, providing a WebGPU-based graphics layer that runs in browsers.

## Prerequisites

1. **Emscripten SDK** installed and activated:
   ```bash
   # If not already installed
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh  # Add to ~/.bashrc for persistence
   ```

2. **CMake** 3.20 or later

3. **Python 3** (for local testing)

## Building with CMake

### Quick Build

Use the provided build script:

```bash
./build_web.sh
```

### Manual Build

```bash
# Create and enter build directory
mkdir -p build_web
cd build_web

# Configure with Emscripten
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_WEBGPU_BACKEND=ON \
    -DBUILD_VULKAN_BACKEND=OFF

# Build
emmake make -j$(nproc)
```

## Output

The build produces web artifacts in `build_web/web/`:
- `cube_example_web.html` - HTML page with canvas
- `cube_example_web.js` - Emscripten JavaScript runtime
- `cube_example_web.wasm` - Compiled WebAssembly binary (~4MB)

## Testing

Start a local web server:

```bash
cd build_web/web
python3 -m http.server 8080
```

Then open in your browser:
```
http://localhost:8080/cube_example_web.html
```

**Note:** WebGPU requires a browser with WebGPU support:
- Chrome/Edge 113+
- Firefox (experimental, enable in `about:config`)

## How It Works

### CMake Configuration

When building with Emscripten, CMake automatically:

1. **Detects Emscripten**: Sets `BUILD_FOR_WEB=TRUE` when `EMSCRIPTEN` is defined
2. **Disables Vulkan**: Forces `BUILD_VULKAN_BACKEND=OFF`
3. **Uses Static Libraries**: Sets `BUILD_SHARED_LIBS=OFF`
4. **Configures Dawn**: Disables Vulkan and X11 for web builds
5. **Sets up Dependencies**: Uses `emdawnwebgpu_headers_gen` for WebGPU headers

### Library Linking

- **Native builds**: Link to `webgpu_dawn` library
- **Web builds**: Use `--use-port=emdawnwebgpu` Emscripten port (no explicit linking)

### Include Paths

Web builds use generated headers from Dawn's Emscripten port:
```
build_web/dawn/gen/src/emdawnwebgpu/include/
```

These are prioritized over source headers to avoid missing `dawn/webgpu.h` issues.

### Example Configuration

The web example target uses these Emscripten flags:

```cmake
target_link_options(cube_example_web PRIVATE
    --use-port=emdawnwebgpu      # WebGPU via Dawn
    -sUSE_GLFW=3                 # GLFW emulation
    -sASYNCIFY=1                 # Async operations support
    -sALLOW_MEMORY_GROWTH=1      # Dynamic memory
    --shell-file=shell.html      # Custom HTML template
)
```

## Shader Compilation

Shaders are compiled from WGSL to SPIR-V at build time using Dawn's `tint` compiler:

```bash
tint shader.wgsl -o shader.spv --format spirv --entry-point main
```

**Note:** tint generates SPIR-V 1.3 or 1.4 (not 1.0). This requires Vulkan 1.1+ for native builds.

## Troubleshooting

### Port already in use
Try a different port: `python3 -m http.server 8181`

### Browser shows "WebGPU not supported"
- Check browser version and enable WebGPU flags if needed
- Ensure HTTPS or localhost (WebGPU security requirement)

### Build fails with "dawn/webgpu.h not found"
Ensure clean rebuild:
```bash
rm -rf build_web
./build_web.sh
```

### Long build times
First build fetches Dawn dependencies (~100s). Subsequent builds are faster.

## Architecture

```
┌─────────────────────────────────────┐
│      Your Application (C/C++)       │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│     GfxWrapper API (gfx.h)          │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   WebGPU Backend (WebGPUBackend.cpp)│
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│    Dawn WebGPU (emdawnwebgpu port)  │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│    Browser WebGPU Implementation    │
│      (Chrome, Firefox, etc.)        │
└─────────────────────────────────────┘
```

## CMake Build System Details

### Key CMakeLists.txt Changes

1. **Emscripten Detection**:
   ```cmake
   if(EMSCRIPTEN)
       set(BUILD_FOR_WEB TRUE)
   endif()
   ```

2. **Conditional Dawn Configuration**:
   ```cmake
   if(BUILD_FOR_WEB)
       set(DAWN_ENABLE_VULKAN OFF CACHE BOOL "" FORCE)
       set(DAWN_USE_X11 OFF CACHE BOOL "" FORCE)
   endif()
   ```

3. **Header Generation Dependencies**:
   ```cmake
   if(TARGET webgpu_headers_gen)
       add_dependencies(gfx webgpu_headers_gen)
   endif()
   if(TARGET emdawnwebgpu_headers_gen)
       add_dependencies(gfx emdawnwebgpu_headers_gen)
   endif()
   ```

4. **Conditional Library Linking**:
   ```cmake
   if(NOT BUILD_FOR_WEB)
       target_link_libraries(gfx PUBLIC webgpu_dawn)
   endif()
   ```

## Comparison: CMake vs Manual Build

| Aspect | CMake (New) | Manual build.sh (Old) |
|--------|-------------|----------------------|
| Build System | Unified (native + web) | Separate scripts |
| Configuration | Automatic detection | Manual flags |
| Dependencies | Handled by CMake | Manual tracking |
| Maintenance | Easier, centralized | Harder, duplicated |
| Flexibility | High (CMAKE_BUILD_TYPE) | Limited |

**The CMake approach is now the recommended way to build for web.**
