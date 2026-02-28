# GFX - Cross-Platform Graphics Abstraction Library

A graphics library providing a low-level Vulkan-like API that works "identically" on both native platforms (via Vulkan/WebGPU) and the web (via WebGPU). Write your rendering code once using modern graphics concepts, and deploy everywhere without modifications - from high-performance desktop/mobile applications to web browsers.

**🌐 [Try the live WebGPU examples in your browser!](https://helcl42.github.io/gfx/)**

## Features

- **Unified API**: Single, consistent API across Vulkan and WebGPU backends(open for other backends like Metal, D3D12)
- **Cross-Platform**: Native support for Android, Windows, macOS, iOS, Linux, and WebAssembly
- **Dual Language APIs**: Idiomatic C and C++ interfaces with identical functionality
- **Production Ready**: Comprehensive documentation, error handling, and validation
- **Modern Architecture**: Command buffers, render passes, compute pipelines, and explicit synchronization
- **Zero-Cost Abstraction**: Thin wrapper with minimal overhead
- **Extension System**: Type-safe extension chains for optional features (multiview, etc.)
- **Thread-Safe**: Device and queue operations are thread-safe for multi-threaded rendering
- **Backend Agnostic**: Write once, run on Vulkan/WebGPU natively or in web(Emscripten) without code changes

## Platform Support

| Platform      | Vulkan | WebGPU | Status | Notes                        |
|---------------|--------|--------|--------|------------------------------|
| Linux x64     | ✅     | ✅     | [![Linux Build](https://github.com/helcl42/GFX/actions/workflows/linux.yml/badge.svg)](https://github.com/helcl42/GFX/actions/workflows/linux.yml)       | X11/Wayland support          |
| Windows x64   | ✅     | ✅     | [![Windows Build](https://github.com/helcl42/GFX/actions/workflows/windows.yml/badge.svg)](https://github.com/helcl42/GFX/actions/workflows/windows.yml)       |                              |
| macOS         | ✅     | ✅     | [![macOS Build](https://github.com/helcl42/GFX/actions/workflows/macos.yml/badge.svg)](https://github.com/helcl42/GFX/actions/workflows/macos.yml)       | Vulkan via MoltenVK          |
| iOS           | ✅     | ✅     | [![iOS Build](https://github.com/helcl42/GFX/actions/workflows/ios.yml/badge.svg)](https://github.com/helcl42/GFX/actions/workflows/ios.yml)       |                              |
| Android       | ✅     | ✅     | [![Android Build](https://github.com/helcl42/GFX/actions/workflows/android.yml/badge.svg)](https://github.com/helcl42/GFX/actions/workflows/android.yml)        |                              |
| WebAssembly   | ❌     | ✅     | [![Web Build](https://github.com/helcl42/GFX/actions/workflows/web.yml/badge.svg)](https://github.com/helcl42/GFX/actions/workflows/web.yml)       | WebGPU only (via Emscripten) |

## Project Structure

```
gfx/
├── README.md
├── CMakeLists.txt
├── cmake/                          # CMake configuration and utilities
├── gfx/                            # C API library
│   ├── include/gfx/
│   │   └── gfx.h                   # Public C API header (2000+ lines)
│   └── src/
│       ├── backend/
│       │   ├── vulkan/             # Vulkan backend implementation
│       │   └── webgpu/             # WebGPU/Dawn backend implementation
│       └── common/                 # Common utilities and helpers
├── gfx_cpp/                        # C++ API library (wraps C API)
│   ├── include/gfx_cpp/
│   │   └── gfx.hpp                 # Public C++ API header (1500+ lines)
│   └── src/
│       ├── core/                   # C++ implementation classes
│       └── converter/              # C++ to C descriptor conversion
├── examples/                       # Example applications
│   ├── c/                          # C API examples
│   │   ├── cube/                   # Rotating 3D cube with textures
│   │   └── compute/                # GPU compute shader example
│   ├── cpp/                        # C++ API examples (same as C but idiomatic)
│   └── web/                        # HTML templates for web builds
├── test/                           # Unit tests (Google Test)
│   ├── gfx/                        # C API tests
│   │   ├── api/                    # Public API tests
│   │   └── internal/               # Backend implementation tests
│   └── gfx_cpp/                    # C++ API tests
│       ├── api/                    # C++ wrapper API tests
│       └── internal/               # Converter and utility tests
├── scripts/                        # Build and utility scripts
│   ├── build_web.sh                # Emscripten build script
│   ├── https_server.py             # HTTPS development server
│   └── setup_https.sh              # Certificate generation
├── third_party/
│   └── dawn/                       # Dawn WebGPU implementation (via CMake FetchContent)
├── build/                          # Native build output (gitignored)
└── build_web/                      # Emscripten build output (gitignored)
```

## Requirements

### Dependencies
- **Vulkan** 1.1+
- **Dawn** WebGPU implementation 
- **GLFW** 3.3+ (for examples only)

## Building

### Quick Start
```bash
# Clone repository with submodules (Dawn WebGPU)
git clone --recurse-submodules https://github.com/helcl42/gfx.git
cd gfx

# Configure and build (Dawn dependencies auto-fetched on first build)
cmake -B build
cmake --build build

# Run examples
./build/cube_example_c
./build/cube_example_cpp
```

### CMake Options
```bash
# Backend selection
cmake -B build -DBUILD_VULKAN_BACKEND=ON   # Enable Vulkan (default: ON)
cmake -B build -DBUILD_WEBGPU_BACKEND=ON   # Enable WebGPU (default: ON)

# Components
cmake -B build -DBUILD_CPP_WRAPPER=ON      # Build C++ API (default: ON)
cmake -B build -DBUILD_EXAMPLES=ON         # Build examples (default: ON)
cmake -B build -DBUILD_TESTS=ON            # Build unit tests (default: ON)

# Library type
cmake -B build -DBUILD_SHARED_LIBS=OFF     # Build static libs (default: ON for shared)
```

### WebAssembly Build

Build for the web using Emscripten:

**🌐 [View live demos](https://helcl42.github.io/gfx/)** - All examples running in your browser via WebGPU!

```bash
# Install Emscripten SDK (one-time setup)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Configure and build for WebAssembly
cd /path/to/gfx
emcmake cmake -B build_web
cmake --build build_web

# Setup HTTPS (required for WebGPU)
./scripts/setup_https.sh  # Generates self-signed certificate

# Serve examples via HTTPS
python3 scripts/https_server.py build_web/web

# Access in browser at https://localhost:8443/
# - cube_example_web.html
# - compute_example_web.html
# - cube_example_cpp_web.html
# - compute_example_cpp_web.html

# Or test on mobile devices (find your local IP first)
# Access at https://YOUR_LOCAL_IP:8443/
# (Accept certificate warning on device)
```

**Why HTTPS?**
- WebGPU requires secure context (HTTPS or localhost)
- Self-signed certificate works for development
- Use `setup_https.sh` to generate certificate for your local IP

**Browser Requirements:**
- Chrome/Edge 113+ with WebGPU enabled
- Firefox Nightly with `dom.webgpu.enabled` flag

### Installation
```bash
# Install to system directories (requires sudo on Linux/macOS)
cmake --build build --target install

# Or specify custom install prefix
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/gfx
cmake --build build --target install
```

Installs:
- Headers: `${PREFIX}/include/gfx/` and `${PREFIX}/include/gfx_cpp/`
- Libraries: `${PREFIX}/lib/`
- CMake config: `${PREFIX}/lib/cmake/gfx/`

## Quick Start

### C API Example
```c
#include <gfx/gfx.h>
#include <stdio.h>

int main() {
    // Create instance with automatic backend selection
    GfxInstanceDescriptor instanceDesc = {
        .sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR,
        .pNext = NULL,
        .backend = GFX_BACKEND_AUTO,
        .applicationName = "My Application",
        .applicationVersion = GFX_MAKE_VERSION(1, 0, 0),
        .enabledExtensions = (const char*[]){GFX_INSTANCE_EXTENSION_SURFACE},
        .enabledExtensionCount = 1
    };
    
    GfxInstance instance;
    GfxResult result = gfxCreateInstance(&instanceDesc, &instance);
    if (result != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %s\n", gfxResultToString(result));
        return -1;
    }

    // Request adapter and create device
    GfxAdapter adapter;
    result = gfxInstanceRequestAdapter(instance, NULL, &adapter);
    
    GfxDevice device;
    result = gfxAdapterCreateDevice(adapter, NULL, &device);
    
    GfxQueue queue;
    result = gfxDeviceGetQueue(device, &queue);

    // Create resources, record commands, render...

    // Cleanup
    gfxDeviceDestroy(device);
    gfxAdapterDestroy(adapter);
    gfxInstanceDestroy(instance);
    
    return 0;
}
```

### C++ API Example
```cpp
#include <gfx_cpp/gfx.hpp>
#include <iostream>

int main() {
    try {
        // Create instance with RAII semantics
        gfx::InstanceDescriptor desc{};
        desc.backend = gfx::Backend::Auto;
        desc.applicationName = "My Application";
        desc.applicationVersion = GFX_MAKE_VERSION(1, 0, 0);
        desc.enabledExtensions = {gfx::INSTANCE_EXTENSION_SURFACE};
        
        auto instance = gfx::createInstance(desc);
        auto adapter = instance->requestAdapter();
        auto device = adapter->createDevice();
        auto queue = device->getQueue();

        // Create resources, record commands, render...
        
        // Automatic cleanup via shared_ptr destructors
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
```

**Key Differences:**
- C API uses explicit error codes (`GfxResult`) and manual cleanup
- C++ API uses exceptions and RAII (`std::shared_ptr`) for automatic cleanup

## API Documentation

### Core Architecture

**Instance** → **Adapter** → **Device** → **Resources**

- **Instance**: Root object representing the graphics system
- **Adapter**: Physical GPU device with capabilities query
- **Device**: Logical device for resource creation
- **Queue**: Command submission and synchronization
- **Resources**: Buffers, textures, shaders, pipelines

All APIs follow this hierarchy consistently across C and C++.

### Extension System

GFX uses a Vulkan-style extension chain system for optional features:

**C API:**
```c
// All structures in pNext chains start with GfxChainHeader
typedef struct GfxChainHeader {
    GfxStructureType sType;
    const void* pNext;
} GfxChainHeader;

// Example: Multiview rendering for VR/stereo
uint32_t mask = 0x3; // Render stereo

GfxRenderPassMultiviewDescriptor multiview = {
    .sType = GFX_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_DESCRIPTOR,
    .pNext = NULL,
    .viewMask = mask,  
    .correlationMasks = &mask,
    .correlationMaskCount = 1
};

GfxRenderPassDescriptor rpDesc = {
    .sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR,
    .pNext = &multiview,  // Chain the extension
    // ... other fields
};
```

**C++ API:**
```cpp
// All extensions inherit from ChainedStruct
struct RenderPassMultiviewDescriptor : public gfx::ChainedStruct {
    uint32_t viewMask = 0;
    std::vector<uint32_t> correlationMasks;
};

RenderPassMultiviewDescriptor multiview{};
multiview.viewMask = 0x3;
multiview.correlationMasks = {0x3};

gfx::RenderPassCreateDescriptor desc{};
desc.next = &multiview;  // Type-safe chaining
```

**Available Extensions:**
- `DEVICE_EXTENSION_MULTIVIEW` - Multi-view rendering (VR/stereo)
- `DEVICE_EXTENSION_TIMELINE_SEMAPHORE` - Timeline semaphores
- `DEVICE_EXTENSION_ANISOTROPIC_FILTERING` - Anisotropic texture filtering

### Threading Model

**Thread-Safe Operations:**
- Device creation and destruction
- Queue submission (`gfxQueueSubmit` / `queue->submit()`)
- Resource creation on device

**Not Thread-Safe:**
- Command encoder recording (use one encoder per thread)
- Resource mapping/unmapping

**Best Practice:**
```cpp
// Per-thread command encoding
std::thread worker([&device]() {
    auto encoder = device->createCommandEncoder();
    // Record commands...
    encoder->end();
});
```

See [gfx/include/gfx/gfx.h](gfx/include/gfx/gfx.h) documentation for complete threading guidelines.

## Error Handling

### C API - Explicit Error Codes
All fallible operations return `GfxResult`:

```c
GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
if (result != GFX_RESULT_SUCCESS) {
    fprintf(stderr, "Buffer creation failed: %s\n", gfxResultToString(result));
    // Handle error...
}
```

**Common Result Codes:**
- `GFX_RESULT_SUCCESS` - Operation succeeded
- `GFX_RESULT_ERROR_OUT_OF_MEMORY` - Allocation failed
- `GFX_RESULT_ERROR_DEVICE_LOST` - GPU crashed or disconnected
- `GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED` - Feature unavailable on this device

See [gfx.h](gfx/include/gfx/gfx.h) for complete error code documentation.

### C++ API - Exceptions
```cpp
try {
    auto buffer = device->createBuffer(desc);
    // Use buffer...
} catch (const std::runtime_error& e) {
    std::cerr << "Buffer creation failed: " << e.what() << std::endl;
}
```

C++ exceptions map to C error codes internally.

## Examples

The library includes complete working examples demonstrating various features:

### Cube Example (`examples/c/cube` and `examples/cpp/cube`)
Renders a rotating 3D cube with:
- Vertex and index buffers
- Uniform buffer transformations
- Depth testing
- Per-vertex color interpolation
- WGSL/SPIR-V shader loading
- Swapchain presentation

```bash
./build/cube_example_c
./build/cube_example_cpp
```

### Compute Example (`examples/c/compute` and `examples/cpp/compute`)
GPU compute shader demonstration:
- Compute pipeline creation
- Storage buffer usage
- Dispatch workgroups
- Fullscreen rendering

```bash
./build/compute_example_c
./build/compute_example_cpp
```

All examples work on native platforms and web (Emscripten).

## Testing

The library includes comprehensive unit tests using Google Test:

```bash
# Run all tests
cd build
ctest

# Run specific test suites
./test/gfx/gfx_api_test                    # C API tests
./test/gfx/gfx_internal_test               # Internal implementation tests
./test/gfx/gfx_internal_vulkan_test        # Vulkan backend tests (if built)
./test/gfx_cpp/gfx_cpp_api_test            # C++ API tests
./test/gfx_cpp/gfx_cpp_internal_test       # C++ wrapper tests

# Run with filter
./test/gfx/gfx_api_test --gtest_filter="InstanceTest.*"
```

See [test/gfx/README.md](test/gfx/README.md) and [test/gfx_cpp/README.md](test/gfx_cpp/README.md) for details.

## Documentation

- **API Reference**: See header files with comprehensive inline documentation
  - [gfx/include/gfx/gfx.h](gfx/include/gfx/gfx.h) - C API (400+ lines of documentation)
  - [gfx_cpp/include/gfx_cpp/gfx.hpp](gfx_cpp/include/gfx_cpp/gfx.hpp) - C++ API
- **Threading Model**: Documented in gfx.h (lines 26-120)
- **Memory Ownership**: Documented in gfx.h (lines 122-240)
- **Error Handling**: Documented in gfx.h (lines 242-349)
- **Examples**: Complete working code in `examples/` directory

## Contributing

Contributions are welcome! Please:

1. Follow the existing code style
2. Add tests for new features
3. Update documentation in header files
4. Ensure all tests pass before submitting
5. Submit pull requests with clear descriptions

## License

MIT License

Copyright (c) 2026 helcl42

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Acknowledgments

- **Vulkan**: Khronos Group Vulkan API
- **WebGPU**: W3C WebGPU specification
- **Dawn**: Google's WebGPU implementation
- **GLFW**: Multi-platform windowing library

### Getting Help

- **Validation**: Enable validation layers for detailed diagnostics
- **Documentation**: Check inline comments in [gfx.h](gfx/include/gfx/gfx.h) and [gfx.hpp](gfx_cpp/include/gfx_cpp/gfx.hpp)
- **Examples**: Review `examples/` directory for complete reference implementations
- **Issues**: Report bugs or request features on GitHub