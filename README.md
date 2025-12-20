# GfxWrapper - Unified Graphics API

A cross-platform graphics abstraction library that provides a unified C and C++ API over Vulkan and WebGPU backends.

## Features

- **Unified API**: Single API that works with both Vulkan and WebGPU backends
- **Cross-Platform**: Windows, macOS, and Linux support
- **Dual Language Support**: Both C and C++ APIs with identical functionality
- **Modern Design**: Uses contemporary graphics API concepts (command buffers, render passes, pipelines)
- **Zero-Cost Abstraction**: Thin wrapper that doesn't add significant overhead
- **Automatic Backend Selection**: Chooses the best available backend automatically
- **Decoupled Backend Loading**: Explicit control over backend API loading and instance lifecycle
- **Extensible Architecture**: Easy to add new backends without modifying dispatcher code
- **Automatic Shader Compilation**: GLSL shaders compiled to SPIR-V during build via CMake

## Project Structure

```
GfxWrapper/
├── README.md
├── CMakeLists.txt           # Build system with shader compilation
├── gfx/                     # C API
│   ├── GfxApi.h            # C API header
│   ├── GfxBackend.h        # Backend interface
│   ├── GfxDispatcher.c     # Runtime dispatcher
│   ├── GfxVulkan.c         # Vulkan backend (C)
│   └── GfxWebGPU.c         # WebGPU backend (C)
├── gfx_cpp/                # C++ API
│   ├── GfxApi.hpp          # C++ API header
│   └── GfxApiImpl.cpp      # C++ wrapper implementation
├── examples/               # Example applications
│   ├── c/
│   │   ├── cube_example.c
│   │   └── shaders/
│   │       ├── cube.vert   # GLSL vertex shader
│   │       └── cube.frag   # GLSL fragment shader
│   └── cpp/
│       └── cube_example.cpp
└── build/                  # Build output
    ├── cube_example_c
    ├── cube_example_cpp
    ├── cube.vert.spv       # Compiled SPIR-V shaders
    └── cube.frag.spv
```

## Requirements

### Build Dependencies
- CMake 3.16 or later
- C11 and C++17 compatible compiler
- GLFW 3.x (for examples)
- Vulkan SDK with `glslc` shader compiler

### Runtime Dependencies
- **Vulkan**: Vulkan SDK installed with drivers
- **WebGPU**: Dawn library (optional, falls back to Vulkan)

### Platform-Specific
- **Windows**: Visual Studio 2019+ or MinGW-w64
- **macOS**: Xcode 11+ with Metal support via MoltenVK
- **Linux**: GCC 7+ or Clang 6+, X11 development libraries

## Building

### Basic Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Build Options
```bash
# Disable WebGPU backend
cmake -DBUILD_WEBGPU_BACKEND=OFF ..

# Disable Vulkan backend
cmake -DBUILD_VULKAN_BACKEND=OFF ..

# Skip examples
cmake -DBUILD_EXAMPLES=OFF ..
```

### Shader Compilation
Shaders are automatically compiled from GLSL to SPIR-V during the build process using `glslc` from the Vulkan SDK. No manual compilation step is needed!

The build system will:
1. Detect `glslc` from your Vulkan SDK installation
2. Compile `.vert` and `.frag` shaders to `.spv` files
3. Place compiled shaders in the build directory
4. Rebuild shaders automatically when source files change

### Installation
```bash
cmake --build . --target install
```

## Quick Start

### C API Example
```c
#include "GfxApi.h"

// Load the backend API first (decoupled from instance creation)
if (!gfxLoadBackend(GFX_BACKEND_AUTO)) {
    fprintf(stderr, "Failed to load graphics backend\n");
    return -1;
}

// Create instance (backend is already loaded)
GfxInstanceDescriptor instanceDesc = {
    .backend = GFX_BACKEND_AUTO,
    .enableValidation = true,
    .applicationName = "My App",
    .applicationVersion = 1
};
GfxInstance instance = gfxCreateInstance(&instanceDesc);

// Get adapter and create device
GfxAdapterDescriptor adapterDesc = {
    .powerPreference = GFX_POWER_PREFERENCE_HIGH_PERFORMANCE
};
GfxAdapter adapter = gfxInstanceRequestAdapter(instance, &adapterDesc);
GfxDevice device = gfxAdapterCreateDevice(adapter, NULL);
GfxQueue queue = gfxDeviceGetQueue(device);

// Create resources...

// Cleanup
gfxDeviceDestroy(device);
gfxAdapterDestroy(adapter);
gfxInstanceDestroy(instance);

// Unload backend API when done
gfxUnloadBackend(GFX_BACKEND_AUTO);
```

### C++ API Example
```cpp
#include "GfxApi.hpp"
using namespace gfx;

// Create instance
InstanceDescriptor desc{};
desc.applicationName = "My App";
desc.enableValidation = true;

auto instance = createInstance(desc);
auto adapter = instance->requestAdapter();
auto device = adapter->createDevice();
auto queue = device->getQueue();

// Create resources...
// C++ destructors handle cleanup automatically
```

## Core Concepts

### Instance
The root object that represents the graphics system. Creates adapters.

### Adapter
Represents a physical graphics device (GPU). Creates logical devices.

### Device
Logical connection to an adapter. Creates all other graphics resources.

#### Device Limits and Buffer Alignment
Modern GPUs have hardware-specific alignment requirements for uniform and storage buffers. The API provides functions to query these limits:

**C API:**
```c
GfxDeviceLimits limits;
gfxDeviceGetLimits(device, &limits);

// Use alignment helpers for buffer offsets
uint64_t offset = calculateSomeOffset();
uint64_t alignedOffset = gfxAlignUp(offset, limits.minUniformBufferOffsetAlignment);

// Common use case: multiple uniform blocks in one buffer
uint64_t block1Offset = 0;
uint64_t block2Offset = gfxAlignUp(block1Size, limits.minUniformBufferOffsetAlignment);
uint64_t block3Offset = gfxAlignUp(block2Offset + block2Size, limits.minUniformBufferOffsetAlignment);
```

**C++ API:**
```cpp
auto limits = device->getLimits();

// Use utils namespace helpers
uint64_t alignedOffset = gfx::utils::alignUp(offset, limits.minUniformBufferOffsetAlignment);
```

**Available Limits:**
- `minUniformBufferOffsetAlignment` - Minimum alignment for uniform buffer offsets (typically 64-256 bytes)
- `minStorageBufferOffsetAlignment` - Minimum alignment for storage buffer offsets (typically 4-32 bytes)
- `maxUniformBufferBindingSize` - Maximum size of a uniform buffer binding
- `maxStorageBufferBindingSize` - Maximum size of a storage buffer binding
- `maxBufferSize` - Maximum total buffer size
- `maxTextureDimension1D/2D/3D` - Maximum texture dimensions
- `maxTextureArrayLayers` - Maximum array layers for texture arrays

**Why This Matters:**
- Dynamic offsets in bind groups MUST be aligned to `minUniformBufferOffsetAlignment`
- Suballocating multiple uniform blocks within a single buffer requires proper alignment
- Failure to align can cause validation errors or GPU faults

### Queue
Submits command buffers for execution on the GPU.

### Resources
- **Buffer**: Linear data storage (vertices, indices, uniforms)
- **Texture**: Image data with filtering and sampling
- **Sampler**: Defines how textures are filtered and addressed
- **Shader**: Programmable shader code (vertex, fragment, compute)

### Pipelines
- **RenderPipeline**: Graphics rendering state and shaders
- **ComputePipeline**: Compute shader execution state

### Command Recording
- **CommandEncoder**: Records GPU commands
- **RenderPassEncoder**: Records rendering commands within a render pass
- **ComputePassEncoder**: Records compute dispatch commands

### Presentation
- **Surface**: Platform window surface for rendering
- **Swapchain**: Manages framebuffers for presentation

### Backend Loading (New!)
Backend APIs are now loaded explicitly and independently from instance creation. This provides better control over the API lifecycle:

#### C API
```c
// Load a specific backend
bool success = gfxLoadBackend(GFX_BACKEND_VULKAN);

// Load all available backends
bool anyLoaded = gfxLoadAllBackends();

// Unload when done
gfxUnloadBackend(GFX_BACKEND_VULKAN);
gfxUnloadAllBackends();
```

**Benefits:**
- **Decoupled Lifecycle**: Backend loading is separate from instance creation
- **Explicit Control**: You decide when to load/unload backend APIs
- **Multiple Instances**: Create multiple instances without reloading the backend
- **Reference Counting**: Backends are automatically managed with ref-counting

**Typical Usage Pattern:**
1. Load backend(s) once at application startup
2. Create one or more instances as needed
3. Destroy instances when done
4. Unload backend(s) at application shutdown

## Architecture

### Handle Metadata System (NEW!)

Instead of trying to dispatch calls to all backends simultaneously, GfxWrapper now uses a **handle metadata** system:

- Every handle returned to users is wrapped with metadata that identifies which backend it belongs to
- Each handle contains:
  - A magic number for validation
  - The backend type (Vulkan, WebGPU, etc.)
  - The actual backend-specific handle

**Benefits:**
- ✅ **Correct Semantics**: Calls are routed to the exact backend that created the handle
- ✅ **No Cross-Backend Confusion**: A Vulkan buffer is always handled by Vulkan, never WebGPU
- ✅ **Easy to Extend**: Adding a new backend requires:
  1. Implementing the backend API functions
  2. Adding a case to `gfxLoadBackend()` and `gfxUnloadBackend()`
  3. That's it! No changes to dispatcher logic needed
- ✅ **Type Safety**: Magic number validation catches invalid handles
- ✅ **Memory Efficient**: Small metadata overhead per handle

### Adding a New Backend

To add a new backend (e.g., Metal, DirectX 12):

1. **Define the backend enum** in `GfxBackend.h`:
   ```c
   typedef enum {
       GFX_BACKEND_VULKAN = 0,
       GFX_BACKEND_WEBGPU = 1,
       GFX_BACKEND_METAL = 2,    // Add here
       GFX_BACKEND_AUTO = 99
   } GfxBackend;
   ```

2. **Implement the backend API** (create `GfxMetal.c`):
   ```c
   const GfxBackendAPI* gfxGetMetalBackend(void) {
       static GfxBackendAPI api = {
           .createInstance = metalCreateInstance,
           .instanceDestroy = metalInstanceDestroy,
           // ... implement all API functions
       };
       return &api;
   }
   ```

3. **Add load/unload cases** in `GfxDispatcher.c`:
   ```c
   case GFX_BACKEND_METAL:
   #ifdef GFX_ENABLE_METAL
       if (!g_backendAPIs[GFX_BACKEND_METAL]) {
           g_backendAPIs[GFX_BACKEND_METAL] = gfxGetMetalBackend();
           // ...
       }
   #endif
   ```

That's it! The handle metadata system automatically routes all calls to the correct backend.

## API Documentation

### Buffer Management
```c
// C API
GfxBufferDescriptor bufferDesc = {
    .size = 1024,
    .usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST,
    .mappedAtCreation = false
};
GfxBuffer buffer = gfxDeviceCreateBuffer(device, &bufferDesc);
```

```cpp
// C++ API
BufferDescriptor desc{};
desc.size = 1024;
desc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
auto buffer = device->createBuffer(desc);
```

### Shader Creation

#### Using SPIR-V Shaders (Recommended)
```c
// Load compiled SPIR-V binary
size_t shaderSize;
void* shaderCode = loadBinaryFile("shader.vert.spv", &shaderSize);

GfxShaderDescriptor shaderDesc = {
    .code = shaderCode,
    .codeSize = shaderSize,
    .entryPoint = "main"  // SPIR-V entry point
};
GfxShader shader = gfxDeviceCreateShader(device, &shaderDesc);
```

```cpp
// C++ API
auto shaderCode = loadBinaryFile("shader.vert.spv");
ShaderDescriptor desc{};
desc.code = std::string(reinterpret_cast<const char*>(shaderCode.data()), 
                        shaderCode.size());
desc.entryPoint = "main";
auto shader = device->createShader(desc);
```

#### GLSL Shader Example
```glsl
// cube.vert - Vertex shader
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform UniformData {
    mat4 model;
    mat4 view;
    mat4 projection;
} uniforms;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = uniforms.projection * uniforms.view * uniforms.model * vec4(position, 1.0);
    fragColor = color;
}
```

### Render Pipeline
```c
// C API
GfxVertexAttribute attributes[] = {
    {.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT, .offset = 0, .shaderLocation = 0}
};
GfxVertexBufferLayout layout = {
    .arrayStride = sizeof(Vertex),
    .attributes = attributes,
    .attributeCount = 1
};
GfxRenderPipelineDescriptor pipelineDesc = {
    .vertex = {.module = vertexShader, .entryPoint = "vs_main", .buffers = &layout, .bufferCount = 1},
    .fragment = &fragmentState,
    .primitive = {.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST}
};
GfxRenderPipeline pipeline = gfxDeviceCreateRenderPipeline(device, &pipelineDesc);
```

### Command Recording and Submission
```c
// C API
GfxCommandEncoder encoder = gfxDeviceCreateCommandEncoder(device, "Render");
GfxRenderPassEncoder renderPass = gfxCommandEncoderBeginRenderPass(encoder, ...);

gfxRenderPassEncoderSetPipeline(renderPass, pipeline);
gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, bufferSize);
gfxRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
gfxRenderPassEncoderEnd(renderPass);

gfxCommandEncoderFinish(encoder);
gfxQueueSubmit(queue, encoder);
```

## Backend Selection

The library automatically selects the best available backend:

1. **GFX_BACKEND_AUTO**: Try Vulkan first, fallback to WebGPU
2. **GFX_BACKEND_VULKAN**: Force Vulkan (fails if not available)
3. **GFX_BACKEND_WEBGPU**: Force WebGPU (fails if not available)

## Platform Support

| Platform | Vulkan | WebGPU | Notes |
|----------|--------|---------|-------|
| Windows  | ✅     | ✅*     | WebGPU via Dawn |
| macOS    | ✅**   | ✅      | **MoltenVK required |
| Linux    | ✅     | ✅*     | X11 and Wayland |

*WebGPU support depends on Dawn availability
**Vulkan on macOS requires MoltenVK

## Error Handling

### C API
Functions return NULL/0 on failure. Check return values:
```c
GfxDevice device = gfxAdapterCreateDevice(adapter, NULL);
if (!device) {
    fprintf(stderr, "Failed to create device\n");
    return -1;
}
```

### C++ API
Uses exceptions for error reporting:
```cpp
try {
    auto device = adapter->createDevice();
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## Examples

### Rotating Cube Example
Both C and C++ examples render a colorful rotating 3D cube with:
- Dual-axis rotation (X and Y)
- Per-vertex colors with gradient interpolation
- Depth testing for proper 3D rendering
- Uniform buffer for transformation matrices
- Back-face culling

**Features demonstrated:**
- Vertex and index buffers
- Uniform buffers with matrix transformations
- SPIR-V shader loading
- Render pipeline creation with depth testing
- Command encoding and submission
- Swapchain presentation
- Keyboard input (ESC to exit)

Run examples:
```bash
cd build
./cube_example_c      # C version
./cube_example_cpp    # C++ version
```

Both examples create an 800x600 window displaying a rotating cube. Press ESC to exit.

## Advanced Usage

### Depth Testing
```c
// Enable depth testing in render pipeline
GfxDepthStencilState depthStencil = {
    .format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT,
    .depthWriteEnabled = true,
    .depthCompare = GFX_COMPARE_FUNCTION_LESS
};

GfxRenderPipelineDescriptor pipelineDesc = {
    // ...existing code...
    .depthStencil = &depthStencil
};
```

### Bind Groups and Layouts
```c
// Create bind group layout for uniform buffers
GfxBindGroupLayoutEntry layoutEntry = {
    .binding = 0,
    .visibility = GFX_SHADER_STAGE_VERTEX,
    .type = GFX_BINDING_TYPE_BUFFER,
    .buffer = {
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(UniformData)
    }
};

GfxBindGroupLayoutDescriptor layoutDesc = {
    .entries = &layoutEntry,
    .entryCount = 1
};

GfxBindGroupLayout layout = gfxDeviceCreateBindGroupLayout(device, &layoutDesc);

// Create bind group
GfxBindGroupEntry entry = {
    .binding = 0,
    .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
    .resource.buffer = {
        .buffer = uniformBuffer,
        .offset = 0,
        .size = sizeof(UniformData)
    }
};

GfxBindGroupDescriptor bindGroupDesc = {
    .layout = layout,
    .entries = &entry,
    .entryCount = 1
};

GfxBindGroup bindGroup = gfxDeviceCreateBindGroup(device, &bindGroupDesc);
```

### Multi-threading
```cpp
// Each device can be used from multiple threads
// Command encoders are not thread-safe - use one per thread
auto encoder1 = device->createCommandEncoder("Thread 1");
auto encoder2 = device->createCommandEncoder("Thread 2");

// Record commands independently on different threads
// Submit to queue when ready (queue is thread-safe)
```

### Validation and Debugging
Enable validation layers during development:
```c
GfxInstanceDescriptor desc = {
    .enableValidation = true  // Enable debug validation
};
```

## Performance Notes

- Use buffer mapping for frequent updates
- Batch draw calls when possible
- Prefer uniform buffers over push constants for large data
- Use appropriate texture formats for your data
- Enable multisampling judiciously (performance cost)

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is provided as-is for educational purposes. See individual backend licenses (Vulkan, WebGPU/Dawn) for their terms.

## Troubleshooting

### Common Issues

**"Failed to load graphics backend"**
- Ensure Vulkan SDK is installed with proper drivers
- Check that Vulkan runtime is in your system PATH
- Verify GPU supports Vulkan 1.1 or higher

**"glslc not found - shaders will not be compiled automatically"**
- Install or update the Vulkan SDK
- Ensure `glslc` is in your PATH or set `VULKAN_SDK` environment variable
- On Linux: `export VULKAN_SDK=/path/to/vulkan/sdk`

**"Failed to create render pipeline" with validation errors**
- Check that bind group layouts match shader declarations
- Verify vertex attribute formats match shader inputs
- Ensure depth/stencil formats are compatible with render targets

**"No suitable adapter found"**
- Update GPU drivers to latest version
- Check hardware compatibility (Vulkan 1.1+ required)
- Try forcing Vulkan backend: `GFX_BACKEND_VULKAN`

**Cube not rendering or appears black**
- Verify shaders compiled successfully (check build output)
- Check that uniform buffer is being updated each frame
- Ensure depth testing is properly configured
- Verify face winding order matches pipeline cull mode

**Compilation errors**
- Ensure C11/C++17 compiler support
- Install required dependencies (GLFW, Vulkan SDK)
- Check CMake configuration output for missing packages

### Debug Information
Enable validation layers for detailed error messages:
```c
GfxInstanceDescriptor desc = {
    .enableValidation = true  // Shows Vulkan validation layer errors
};
```

### Platform-Specific Notes

**Linux:**
- X11 and Wayland are both supported
- May need to install: `libx11-dev`, `libxcb1-dev`, `libwayland-dev`
- Vulkan validation layers: `apt install vulkan-validationlayers`

**macOS:**
- Requires MoltenVK for Vulkan support
- Install Vulkan SDK which includes MoltenVK
- Metal is the native API on macOS

**Windows:**
- Vulkan SDK installs required runtime components
- Ensure graphics drivers are up to date
- Visual Studio 2019+ recommended for best compatibility

### Getting Help
- Check the cube examples for complete reference implementations
- Review API documentation in `GfxApi.h` and `GfxApi.hpp`
- Enable validation layers for detailed error diagnostics
- Submit issues on GitHub for bugs or feature requests