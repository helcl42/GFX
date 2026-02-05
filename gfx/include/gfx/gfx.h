#ifndef GFX_GFX_H
#define GFX_GFX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// API Version - Semantic Versioning (https://semver.org/)
// ============================================================================

// MAJOR version: Incompatible API changes
// MINOR version: Added functionality (backwards compatible)
// PATCH version: Bug fixes (backwards compatible)
#define GFX_VERSION_MAJOR 1
#define GFX_VERSION_MINOR 0
#define GFX_VERSION_PATCH 0

// Helper macro to create version number for comparison (Vulkan-style bit packing)
// Bits 31-22: major (10 bits, 0-1023), Bits 21-12: minor (10 bits, 0-1023), Bits 11-0: patch (12 bits, 0-4095)
// Example: #if GFX_VERSION < GFX_MAKE_VERSION(1, 2, 0)
#define GFX_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))

// Combined version number for comparison
#define GFX_VERSION GFX_MAKE_VERSION(GFX_VERSION_MAJOR, GFX_VERSION_MINOR, GFX_VERSION_PATCH)

// ============================================================================
// Common Constants
// ============================================================================

// Special timeout value for infinite wait (used with gfxFenceWait, gfxSemaphoreWait)
#define GFX_TIMEOUT_INFINITE UINT64_MAX

// Special size value to map entire buffer from offset (used with gfxBufferMap)
#define GFX_WHOLE_SIZE 0

// ============================================================================
// ERROR HANDLING
// ============================================================================
//
// RETURN CODES:
//
// All GFX functions return GfxResult (except void functions like gfxSetLogCallback)
// 
// Success codes (>= 0):
//   GFX_RESULT_SUCCESS = 0      → Operation completed successfully
//   GFX_RESULT_TIMEOUT = 1      → Operation timed out (not necessarily an error)
//   GFX_RESULT_NOT_READY = 2    → Resource not ready yet (poll again)
//
// Error codes (< 0):
//   GFX_RESULT_ERROR_INVALID_ARGUMENT = -1        → Bad parameter
//   GFX_RESULT_ERROR_NOT_FOUND = -2               → Resource doesn't exist
//   GFX_RESULT_ERROR_OUT_OF_MEMORY = -3           → Allocation failed
//   GFX_RESULT_ERROR_DEVICE_LOST = -4             → GPU crashed or disconnected
//   GFX_RESULT_ERROR_SURFACE_LOST = -5            → Window was destroyed
//   GFX_RESULT_ERROR_OUT_OF_DATE = -6             → Swapchain needs recreation
//   GFX_RESULT_ERROR_BACKEND_NOT_LOADED = -7      → Backend DLL not available
//   GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED = -8   → Feature not available
//   GFX_RESULT_ERROR_UNKNOWN = -9                 → Internal error
//
// CHECKING ERRORS:
//
// Minimal (production):
//   GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
//   if (result != GFX_RESULT_SUCCESS) {
//       // Handle error
//   }
//
// Detailed (debugging):
//   GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
//   if (result != GFX_RESULT_SUCCESS) {
//       fprintf(stderr, "Buffer creation failed: %s\n", gfxResultToString(result));
//       return false;
//   }
//
// RECOVERABLE vs FATAL:
//
// Recoverable errors (retry or fallback possible):
//   GFX_RESULT_TIMEOUT              → Wait longer or skip frame
//   GFX_RESULT_NOT_READY            → Poll again
//   GFX_RESULT_ERROR_OUT_OF_DATE    → Recreate swapchain
//   GFX_RESULT_ERROR_SURFACE_LOST   → Recreate surface
//   GFX_RESULT_ERROR_OUT_OF_MEMORY  → Free resources and retry
//
// Fatal errors (should terminate or restart):
//   GFX_RESULT_ERROR_DEVICE_LOST    → GPU crashed - reset device
//   GFX_RESULT_ERROR_BACKEND_NOT_LOADED → Missing DLL - can't continue
//   GFX_RESULT_ERROR_INVALID_ARGUMENT → Programming error - fix code
//
// COMMON SCENARIOS:
//
// 1. Initialization failure:
//   GfxResult result = gfxCreateInstance(&desc, &instance);
//   if (result == GFX_RESULT_ERROR_BACKEND_NOT_LOADED) {
//       printf("Vulkan/WebGPU not available\n");
//       return EXIT_FAILURE;
//   }
//
// 2. Out of memory:
//   GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
//   if (result == GFX_RESULT_ERROR_OUT_OF_MEMORY) {
//       // Free some resources
//       gfxTextureDestroy(oldTexture);
//       // Retry with smaller buffer
//       desc.size /= 2;
//       result = gfxDeviceCreateBuffer(device, &desc, &buffer);
//   }
//
// 3. Swapchain out of date:
//   GfxResult result = gfxSwapchainAcquireNextImage(...);
//   if (result == GFX_RESULT_ERROR_OUT_OF_DATE) {
//       gfxSwapchainDestroy(swapchain);
//       // Query new window size
//       swapchain = gfxDeviceCreateSwapchain(device, &newDesc);
//   }
//
// 4. Device lost (GPU crash):
//   GfxResult result = gfxQueueSubmit(queue, &submitDesc);
//   if (result == GFX_RESULT_ERROR_DEVICE_LOST) {
//       // GPU crashed - need to recreate device
//       cleanupAllResources();
//       gfxDeviceDestroy(device);
//       device = gfxAdapterCreateDevice(adapter, &deviceDesc);
//       recreateAllResources();
//   }
//
// 5. Fence timeout:
//   GfxResult result = gfxFenceWait(fence, 1000000000); // 1 second
//   if (result == GFX_RESULT_TIMEOUT) {
//       printf("GPU taking longer than expected\n");
//       result = gfxFenceWait(fence, UINT64_MAX); // Wait forever
//   }
//
// VALIDATION:
//
// In debug builds, enable logging to catch issues early:
//   gfxSetLogCallback(myLogFunction, userData);
//   // Library will log warnings for:
//   // - Invalid parameters
//   // - Missing required fields
//   // - Incorrect usage patterns
//   // - Performance warnings
//
// BEST PRACTICES:
//
// 1. Always check return codes for creation functions:
//    ✓ gfxCreateInstance, gfxDeviceCreateBuffer, etc.
//
// 2. Can skip checks for simple setters (but still good to check):
//    ~ gfxRenderPassEncoderSetViewport (unlikely to fail)
//    ~ gfxCommandEncoderEnd (only fails on programming errors)
//
// 3. Always check queue operations:
//    ✓ gfxQueueSubmit (can fail due to device lost)
//    ✓ gfxSwapchainAcquireNextImage (can be out of date)
//
// 4. Use macros for repetitive checking:
//    (Example - use without backslashes in actual code)
//    #define CHECK_RESULT(expr) do {
//        GfxResult _r = (expr);
//        if (_r != GFX_RESULT_SUCCESS) {
//            fprintf(stderr, "%s failed: %s\n", #expr, gfxResultToString(_r));
//            return false;
//        }
//    } while(0)
//
//    CHECK_RESULT(gfxDeviceCreateBuffer(device, &desc, &buffer));

// ============================================================================
// MEMORY OWNERSHIP AND LIFETIME
// ============================================================================
//
// HANDLES (Opaque Pointers):
// - All GfxXxx handles (GfxBuffer, GfxTexture, etc.) are OWNED by the application
// - The library does NOT retain references to handles after a function returns
// - The application MUST call gfxXxxDestroy() to free resources
// - Calling destroy twice on the same handle is UNDEFINED BEHAVIOR
//
// STRINGS:
//
// Input Strings (Borrowed):
//   const char* label = "MyBuffer";
//   gfxDeviceCreateBuffer(device, &desc);
//   → label string is COPIED internally, application can free immediately after call returns
//   → All const char* parameters in descriptors are borrowed and copied
//   → Lifetime requirement: Valid only during the function call
//
// Output Strings (Library-Owned):
//   GfxAdapterInfo info;
//   gfxAdapterGetInfo(adapter, &info);
//   printf("%s", info.name); // OK
//   → info.name is owned by the ADAPTER object
//   → Remains valid until gfxAdapterDestroy() is called
//   → Application MUST NOT free these strings
//   → Application MUST copy if needed after adapter destruction
//
// ARRAYS IN DESCRIPTORS:
//
// All arrays are BORROWED during function call:
//
//   GfxVertexAttribute attrs[3] = {...};
//   GfxVertexBufferLayout layout = {
//       .attributes = attrs,
//       .attributeCount = 3
//   };
//   gfxDeviceCreatePipeline(device, &pipelineDesc);
//   // attrs can be freed or go out of scope here - array was copied
//
// Extension Chains (pNext):
//   GfxSomeExtension ext = { .sType = GFX_STRUCTURE_TYPE_SOME_EXTENSION };
//   GfxDescriptor desc = { .pNext = &ext };
//   gfxCreateSomething(device, &desc);
//   → Extension chain is TRAVERSED and COPIED during the call
//   → Extension structures can be stack-allocated
//   → Lifetime requirement: Valid only during the function call
//
// COMMAND ENCODERS:
//
//   GfxCommandEncoder encoder;
//   gfxDeviceCreateCommandEncoder(device, &desc, &encoder);
//   gfxCommandEncoderCopyBuffer(encoder, ...);
//   gfxCommandEncoderEnd(encoder);
//   gfxQueueSubmit(queue, &submitDesc); // submitDesc contains encoder
//   → Encoder is BORROWED by gfxQueueSubmit (reads commands, doesn't store encoder)
//   → After submit returns, you can destroy the encoder
//   → Encoder must remain valid during the submit call
//   → Commands are copied to internal GPU command buffer
//
// RESOURCE DEPENDENCIES:
//
// Pipelines reference other objects:
//   GfxRenderPipeline pipeline = create with shader, renderPass, layout
//   → Pipeline does NOT own shader/renderPass/layout
//   → Application must keep shader/renderPass/layout alive while pipeline exists
//   → Destroying shader before pipeline is UNDEFINED BEHAVIOR
//
// Bind groups reference resources:
//   GfxBindGroup bindGroup = create with buffer/texture/sampler
//   → Bind group does NOT own the buffer/texture/sampler
//   → Resources must outlive the bind group
//   → Using a bind group after destroying its resources is UNDEFINED BEHAVIOR
//
// Framebuffers reference textures:
//   GfxFramebuffer framebuffer = create with texture views
//   → Framebuffer does NOT own the texture views
//   → Texture views must outlive the framebuffer
//
// GPU SYNCHRONIZATION:
//
//   gfxQueueSubmit(queue, &submitDesc);
//   gfxBufferDestroy(buffer); // DANGER!
//   → Submit is ASYNCHRONOUS - GPU may still be using buffer
//   → Application MUST wait for GPU completion before destroying:
//
//   gfxQueueSubmit(queue, &submitDesc);
//   gfxFenceWait(submitDesc.signalFence, UINT64_MAX);
//   gfxBufferDestroy(buffer); // Now safe
//
// MAPPING LIFETIME:
//
//   void* ptr;
//   gfxBufferMap(buffer, 0, size, &ptr);
//   // ptr is valid until unmap
//   memcpy(ptr, data, size);
//   gfxBufferUnmap(buffer);
//   // ptr is now INVALID - accessing it is undefined behavior
//   → Only one map per buffer at a time
//   → Destroying a mapped buffer is UNDEFINED BEHAVIOR
//
// SUMMARY TABLE:
//
// | Object Type        | Owned By      | Freed By                    | Dependencies      |
// |--------------------|---------------|-----------------------------|--------------------|
// | GfxInstance        | Application   | gfxInstanceDestroy()        | None              |
// | GfxAdapter         | Instance      | gfxAdapterDestroy()         | Instance          |
// | GfxDevice          | Adapter       | gfxDeviceDestroy()          | Adapter           |
// | GfxBuffer          | Device        | gfxBufferDestroy()          | Device            |
// | GfxTexture         | Device        | gfxTextureDestroy()         | Device            |
// | GfxPipeline        | Device        | gfxPipelineDestroy()        | Device, Shader    |
// | GfxBindGroup       | Device        | gfxBindGroupDestroy()       | Device, Resources |
// | Descriptor strings | Application   | Application manages         | N/A               |
// | Info strings       | Parent object | Parent's destroy function   | Parent object     |

// ============================================================================
// THREADING MODEL
// ============================================================================
//
// GENERAL RULES:
// - All GFX functions are thread-safe for operations on DIFFERENT objects
// - Operations on the SAME object from multiple threads require external synchronization
// - Exception: Queue submission has internal synchronization (see below)
//
// DETAILED GUARANTEES:
//
// Instance/Adapter/Device Creation (Thread-Safe):
//   ✓ gfxCreateInstance() - Can be called from any thread
//   ✓ gfxInstanceRequestAdapter() - Can be called concurrently for same instance
//   ✓ gfxAdapterCreateDevice() - Can be called concurrently for same adapter
//
// Resource Creation (Requires External Sync on Same Device):
//   ✗ gfxDeviceCreateBuffer(device, ...)  
//   ✗ gfxDeviceCreateTexture(device, ...)
//   ✗ gfxDeviceCreatePipeline(device, ...)
//   → Multiple threads creating resources on the SAME device must synchronize externally
//   → Different devices can be used concurrently without sync
//
// Command Recording (NOT Thread-Safe per Encoder):
//   ✗ GfxCommandEncoder - Each encoder can only be used by ONE thread at a time
//   ✗ GfxRenderPassEncoder - Each encoder can only be used by ONE thread at a time
//   ✗ GfxComputePassEncoder - Each encoder can only be used by ONE thread at a time
//   → Create one encoder per thread if recording commands in parallel
//   → Example: Thread A records to encoderA, Thread B records to encoderB (OK)
//   → Example: Thread A and B both call commands on encoderA (NOT OK - undefined behavior)
//
// Queue Operations (Thread-Safe):
//   ✓ gfxQueueSubmit() - Internal synchronization, safe to call from multiple threads
//   ✓ gfxQueueWriteBuffer() - Internal synchronization
//   ✓ gfxQueueWriteTexture() - Internal synchronization
//   → The implementation uses a mutex internally for queue operations
//   → Multiple threads can submit to the same queue simultaneously
//
// Resource Destruction (Requires External Sync):
//   ✗ gfxBufferDestroy(buffer)
//   ✗ gfxTextureDestroy(texture)
//   → Application must ensure no other thread is using the object
//   → Application must ensure GPU has finished using the object (use fences)
//   → Destroying an object while the GPU is using it is undefined behavior
//
// Synchronization Objects (Thread-Safe):
//   ✓ gfxFenceWait() - Can be called from multiple threads on same fence
//   ✓ gfxFenceGetStatus() - Thread-safe query
//   ✓ gfxSemaphoreSignal() - Thread-safe for timeline semaphores
//   ✓ gfxSemaphoreWait() - Thread-safe for timeline semaphores
//
// COMMON PATTERNS:
//
// Pattern 1: Multi-threaded resource loading
//   Thread 1: encoder1 = createEncoder(device); // Externally synchronized
//   Thread 2: encoder2 = createEncoder(device); // Externally synchronized
//   Thread 1: record commands to encoder1
//   Thread 2: record commands to encoder2
//   Main:     submit both encoders to queue (queue handles sync internally)
//
// Pattern 2: Parallel command recording
//   Main:     encoders[N] = create N encoders (with mutex on device)
//   Threads:  Each thread records to its own encoder (no sync needed)
//   Main:     gfxQueueSubmit(queue, encoders, N); (thread-safe)
//
// Pattern 3: Producer-consumer with timeline semaphores
//   Thread A: submit work, signal semaphore value 1
//   Thread B: wait for semaphore value 1, submit dependent work
//   → Both threads can call gfxSemaphoreWait/Signal safely
//
// CALLBACKS:
// - GfxLogCallback may be called from ANY thread, including internal library threads
// - Log callbacks must be thread-safe if they access shared state
// - Log callbacks should not call GFX functions (may deadlock)
//
// BACKEND DIFFERENCES:
// - Vulkan: All thread-safety guarantees are honored
// - WebGPU: Additional internal synchronization may occur (still safe to follow rules above)

// ============================================================================
// DLL Export/Import Macros for Windows
// ============================================================================

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef GFX_BUILDING_DLL
// Building the DLL - export symbols
#ifdef __GNUC__
#define GFX_API __attribute__((dllexport))
#else
#define GFX_API __declspec(dllexport)
#endif
#else
// Using the DLL - import symbols
#ifdef __GNUC__
#define GFX_API __attribute__((dllimport))
#else
#define GFX_API __declspec(dllimport)
#endif
#endif
#else
// Non-Windows platforms - use visibility attributes for shared libraries
#if defined(__GNUC__) && __GNUC__ >= 4
#define GFX_API __attribute__((visibility("default")))
#else
#define GFX_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Core Enumerations
// ============================================================================

typedef enum {
    GFX_BACKEND_VULKAN = 0,
    GFX_BACKEND_WEBGPU = 1,
    GFX_BACKEND_AUTO = 2,
    GFX_BACKEND_MAX_ENUM = 0x7FFFFFFF
} GfxBackend;

typedef enum {
    GFX_ADAPTER_TYPE_DISCRETE_GPU = 0,
    GFX_ADAPTER_TYPE_INTEGRATED_GPU = 1,
    GFX_ADAPTER_TYPE_CPU = 2,
    GFX_ADAPTER_TYPE_UNKNOWN = 3,
    GFX_ADAPTER_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxAdapterType;

typedef enum {
    GFX_ADAPTER_PREFERENCE_UNDEFINED = 0,
    GFX_ADAPTER_PREFERENCE_LOW_POWER = 1,
    GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE = 2,
    GFX_ADAPTER_PREFERENCE_SOFTWARE = 3,
    GFX_ADAPTER_PREFERENCE_MAX_ENUM = 0x7FFFFFFF
} GfxAdapterPreference;

typedef enum {
    GFX_PRESENT_MODE_IMMEDIATE = 0,
    GFX_PRESENT_MODE_FIFO = 1,
    GFX_PRESENT_MODE_FIFO_RELAXED = 2,
    GFX_PRESENT_MODE_MAILBOX = 3,
    GFX_PRESENT_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxPresentMode;

typedef enum {
    GFX_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    GFX_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    GFX_PRIMITIVE_TOPOLOGY_MAX_ENUM = 0x7FFFFFFF
} GfxPrimitiveTopology;

typedef enum {
    GFX_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    GFX_FRONT_FACE_CLOCKWISE = 1,
    GFX_FRONT_FACE_MAX_ENUM = 0x7FFFFFFF
} GfxFrontFace;

typedef enum {
    GFX_CULL_MODE_NONE = 0,
    GFX_CULL_MODE_FRONT = 1,
    GFX_CULL_MODE_BACK = 2,
    GFX_CULL_MODE_FRONT_AND_BACK = 3,
    GFX_CULL_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxCullMode;

typedef enum {
    GFX_LOG_LEVEL_ERROR = 0,
    GFX_LOG_LEVEL_WARNING = 1,
    GFX_LOG_LEVEL_INFO = 2,
    GFX_LOG_LEVEL_DEBUG = 3,
    GFX_LOG_LEVEL_MAX_ENUM = 0x7FFFFFFF
} GfxLogLevel;

typedef enum {
    GFX_POLYGON_MODE_FILL = 0,
    GFX_POLYGON_MODE_LINE = 1,
    GFX_POLYGON_MODE_POINT = 2,
    GFX_POLYGON_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxPolygonMode;

typedef enum {
    GFX_INDEX_FORMAT_UNDEFINED = 0,
    GFX_INDEX_FORMAT_UINT16 = 1,
    GFX_INDEX_FORMAT_UINT32 = 2,
    GFX_INDEX_FORMAT_MAX_ENUM = 0x7FFFFFFF
} GfxIndexFormat;

typedef enum {
    GFX_VERTEX_STEP_MODE_VERTEX = 0,
    GFX_VERTEX_STEP_MODE_INSTANCE = 1,
    GFX_VERTEX_STEP_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxVertexStepMode;

typedef enum {
    GFX_TEXTURE_FORMAT_UNDEFINED = 0,
    GFX_TEXTURE_FORMAT_R8_UNORM = 1,
    GFX_TEXTURE_FORMAT_R8G8_UNORM = 2,
    GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM = 3,
    GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB = 4,
    GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM = 5,
    GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB = 6,
    GFX_TEXTURE_FORMAT_R16_FLOAT = 7,
    GFX_TEXTURE_FORMAT_R16G16_FLOAT = 8,
    GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT = 9,
    GFX_TEXTURE_FORMAT_R32_FLOAT = 10,
    GFX_TEXTURE_FORMAT_R32G32_FLOAT = 11,
    GFX_TEXTURE_FORMAT_R32G32B32_FLOAT = 12,
    GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT = 13,
    GFX_TEXTURE_FORMAT_DEPTH16_UNORM = 14,
    GFX_TEXTURE_FORMAT_DEPTH24_PLUS = 15,
    GFX_TEXTURE_FORMAT_DEPTH32_FLOAT = 16,
    GFX_TEXTURE_FORMAT_STENCIL8 = 17,
    GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8 = 18,
    GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8 = 19,
    GFX_TEXTURE_FORMAT_MAX_ENUM = 0x7FFFFFFF
} GfxTextureFormat;

typedef enum {
    GFX_TEXTURE_TYPE_1D = 0,
    GFX_TEXTURE_TYPE_2D = 1,
    GFX_TEXTURE_TYPE_3D = 2,
    GFX_TEXTURE_TYPE_CUBE = 3,
    GFX_TEXTURE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureType;

typedef enum {
    GFX_TEXTURE_VIEW_TYPE_1D = 0,
    GFX_TEXTURE_VIEW_TYPE_2D = 1,
    GFX_TEXTURE_VIEW_TYPE_3D = 2,
    GFX_TEXTURE_VIEW_TYPE_CUBE = 3,
    GFX_TEXTURE_VIEW_TYPE_1D_ARRAY = 4,
    GFX_TEXTURE_VIEW_TYPE_2D_ARRAY = 5,
    GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY = 6,
    GFX_TEXTURE_VIEW_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureViewType;

typedef enum {
    GFX_TEXTURE_SAMPLE_TYPE_FLOAT = 0,
    GFX_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT = 1,
    GFX_TEXTURE_SAMPLE_TYPE_DEPTH = 2,
    GFX_TEXTURE_SAMPLE_TYPE_SINT = 3,
    GFX_TEXTURE_SAMPLE_TYPE_UINT = 4,
    GFX_TEXTURE_SAMPLE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureSampleType;

typedef enum {
    GFX_TEXTURE_USAGE_NONE = 0,
    GFX_TEXTURE_USAGE_COPY_SRC = 1 << 0,
    GFX_TEXTURE_USAGE_COPY_DST = 1 << 1,
    GFX_TEXTURE_USAGE_TEXTURE_BINDING = 1 << 2,
    GFX_TEXTURE_USAGE_STORAGE_BINDING = 1 << 3,
    GFX_TEXTURE_USAGE_RENDER_ATTACHMENT = 1 << 4,
    GFX_TEXTURE_USAGE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureUsageFlagBits;
typedef uint32_t GfxTextureUsageFlags;

typedef enum {
    GFX_TEXTURE_LAYOUT_UNDEFINED = 0, // Initial layout, contents undefined
    GFX_TEXTURE_LAYOUT_GENERAL = 1, // General purpose, can be used for anything but may be slow
    GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT = 2, // Optimal for color render target
    GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT = 3, // Optimal for depth/stencil render target
    GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY = 4, // Optimal for reading depth/stencil
    GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY = 5, // Optimal for sampling in shaders
    GFX_TEXTURE_LAYOUT_TRANSFER_SRC = 6, // Optimal for copy source
    GFX_TEXTURE_LAYOUT_TRANSFER_DST = 7, // Optimal for copy destination
    GFX_TEXTURE_LAYOUT_PRESENT_SRC = 8, // Optimal for presentation
    GFX_TEXTURE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
} GfxTextureLayout;

typedef enum {
    GFX_PIPELINE_STAGE_NONE = 0,
    GFX_PIPELINE_STAGE_TOP_OF_PIPE = 0x00000001,
    GFX_PIPELINE_STAGE_DRAW_INDIRECT = 0x00000002,
    GFX_PIPELINE_STAGE_VERTEX_INPUT = 0x00000004,
    GFX_PIPELINE_STAGE_VERTEX_SHADER = 0x00000008,
    GFX_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER = 0x00000010,
    GFX_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER = 0x00000020,
    GFX_PIPELINE_STAGE_GEOMETRY_SHADER = 0x00000040,
    GFX_PIPELINE_STAGE_FRAGMENT_SHADER = 0x00000080,
    GFX_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS = 0x00000100,
    GFX_PIPELINE_STAGE_LATE_FRAGMENT_TESTS = 0x00000200,
    GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT = 0x00000400,
    GFX_PIPELINE_STAGE_COMPUTE_SHADER = 0x00000800,
    GFX_PIPELINE_STAGE_TRANSFER = 0x00001000,
    GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE = 0x00002000,
    GFX_PIPELINE_STAGE_ALL_GRAPHICS = 0x0000FFFF,
    GFX_PIPELINE_STAGE_ALL_COMMANDS = 0x00010000,
    GFX_PIPELINE_STAGE_MAX_ENUM = 0x7FFFFFFF
} GfxPipelineStageFlagBits;
typedef uint32_t GfxPipelineStageFlags;

typedef enum {
    GFX_ACCESS_NONE = 0,
    GFX_ACCESS_INDIRECT_COMMAND_READ = 1 << 0, // 0x01
    GFX_ACCESS_INDEX_READ = 1 << 1, // 0x02
    GFX_ACCESS_VERTEX_ATTRIBUTE_READ = 1 << 2, // 0x04
    GFX_ACCESS_UNIFORM_READ = 1 << 3, // 0x08
    GFX_ACCESS_INPUT_ATTACHMENT_READ = 1 << 4, // 0x10
    GFX_ACCESS_SHADER_READ = 1 << 5, // 0x20
    GFX_ACCESS_SHADER_WRITE = 1 << 6, // 0x40
    GFX_ACCESS_COLOR_ATTACHMENT_READ = 1 << 7, // 0x80
    GFX_ACCESS_COLOR_ATTACHMENT_WRITE = 1 << 8, // 0x100
    GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ = 1 << 9, // 0x200
    GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE = 1 << 10, // 0x400
    GFX_ACCESS_TRANSFER_READ = 1 << 11, // 0x800
    GFX_ACCESS_TRANSFER_WRITE = 1 << 12, // 0x1000
    GFX_ACCESS_MEMORY_READ = 1 << 14, // 0x4000
    GFX_ACCESS_MEMORY_WRITE = 1 << 15, // 0x8000
    GFX_ACCESS_MAX_ENUM = 0x7FFFFFFF
} GfxAccessFlagBits;
typedef uint32_t GfxAccessFlags;

typedef enum {
    GFX_BUFFER_USAGE_NONE = 0,
    GFX_BUFFER_USAGE_MAP_READ = 1 << 0,
    GFX_BUFFER_USAGE_MAP_WRITE = 1 << 1,
    GFX_BUFFER_USAGE_COPY_SRC = 1 << 2,
    GFX_BUFFER_USAGE_COPY_DST = 1 << 3,
    GFX_BUFFER_USAGE_INDEX = 1 << 4,
    GFX_BUFFER_USAGE_VERTEX = 1 << 5,
    GFX_BUFFER_USAGE_UNIFORM = 1 << 6,
    GFX_BUFFER_USAGE_STORAGE = 1 << 7,
    GFX_BUFFER_USAGE_INDIRECT = 1 << 8,
    GFX_BUFFER_USAGE_MAX_ENUM = 0x7FFFFFFF
} GfxBufferUsageFlagBits;
typedef uint32_t GfxBufferUsageFlags;

typedef enum {
    GFX_MEMORY_PROPERTY_DEVICE_LOCAL = 1 << 0,
    GFX_MEMORY_PROPERTY_HOST_VISIBLE = 1 << 1,
    GFX_MEMORY_PROPERTY_HOST_COHERENT = 1 << 2,
    GFX_MEMORY_PROPERTY_HOST_CACHED = 1 << 3,
    GFX_MEMORY_PROPERTY_MAX_ENUM = 0x7FFFFFFF
} GfxMemoryPropertyFlagBits;
typedef uint32_t GfxMemoryPropertyFlags;

typedef enum {
    GFX_SHADER_STAGE_NONE = 0,
    GFX_SHADER_STAGE_VERTEX = 1 << 0,
    GFX_SHADER_STAGE_FRAGMENT = 1 << 1,
    GFX_SHADER_STAGE_COMPUTE = 1 << 2,
    GFX_SHADER_STAGE_MAX_ENUM = 0x7FFFFFFF
} GfxShaderStageFlagBits;
typedef uint32_t GfxShaderStageFlags;

typedef enum {
    GFX_QUEUE_FLAG_NONE = 0,
    GFX_QUEUE_FLAG_GRAPHICS = 1 << 0,
    GFX_QUEUE_FLAG_COMPUTE = 1 << 1,
    GFX_QUEUE_FLAG_TRANSFER = 1 << 2,
    GFX_QUEUE_FLAG_SPARSE_BINDING = 1 << 3,
    GFX_QUEUE_FLAG_MAX_ENUM = 0x7FFFFFFF
} GfxQueueFlagBits;
typedef uint32_t GfxQueueFlags;

typedef enum {
    GFX_FILTER_MODE_NEAREST = 0,
    GFX_FILTER_MODE_LINEAR = 1,
    GFX_FILTER_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxFilterMode;

typedef enum {
    GFX_ADDRESS_MODE_REPEAT = 0,
    GFX_ADDRESS_MODE_MIRROR_REPEAT = 1,
    GFX_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    GFX_ADDRESS_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxAddressMode;

typedef enum {
    GFX_COMPARE_FUNCTION_UNDEFINED = 0,
    GFX_COMPARE_FUNCTION_NEVER = 1,
    GFX_COMPARE_FUNCTION_LESS = 2,
    GFX_COMPARE_FUNCTION_EQUAL = 3,
    GFX_COMPARE_FUNCTION_LESS_EQUAL = 4,
    GFX_COMPARE_FUNCTION_GREATER = 5,
    GFX_COMPARE_FUNCTION_NOT_EQUAL = 6,
    GFX_COMPARE_FUNCTION_GREATER_EQUAL = 7,
    GFX_COMPARE_FUNCTION_ALWAYS = 8,
    GFX_COMPARE_FUNCTION_MAX_ENUM = 0x7FFFFFFF
} GfxCompareFunction;

typedef enum {
    GFX_BLEND_OPERATION_ADD = 0,
    GFX_BLEND_OPERATION_SUBTRACT = 1,
    GFX_BLEND_OPERATION_REVERSE_SUBTRACT = 2,
    GFX_BLEND_OPERATION_MIN = 3,
    GFX_BLEND_OPERATION_MAX = 4,
    GFX_BLEND_OPERATION_MAX_ENUM = 0x7FFFFFFF
} GfxBlendOperation;

typedef enum {
    GFX_BLEND_FACTOR_ZERO = 0,
    GFX_BLEND_FACTOR_ONE = 1,
    GFX_BLEND_FACTOR_SRC = 2,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC = 3,
    GFX_BLEND_FACTOR_SRC_ALPHA = 4,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 5,
    GFX_BLEND_FACTOR_DST = 6,
    GFX_BLEND_FACTOR_ONE_MINUS_DST = 7,
    GFX_BLEND_FACTOR_DST_ALPHA = 8,
    GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED = 10,
    GFX_BLEND_FACTOR_CONSTANT = 11,
    GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT = 12,
    GFX_BLEND_FACTOR_MAX_ENUM = 0x7FFFFFFF
} GfxBlendFactor;

typedef enum {
    GFX_STENCIL_OPERATION_KEEP = 0,
    GFX_STENCIL_OPERATION_ZERO = 1,
    GFX_STENCIL_OPERATION_REPLACE = 2,
    GFX_STENCIL_OPERATION_INCREMENT_CLAMP = 3,
    GFX_STENCIL_OPERATION_DECREMENT_CLAMP = 4,
    GFX_STENCIL_OPERATION_INVERT = 5,
    GFX_STENCIL_OPERATION_INCREMENT_WRAP = 6,
    GFX_STENCIL_OPERATION_DECREMENT_WRAP = 7,
    GFX_STENCIL_OPERATION_MAX_ENUM = 0x7FFFFFFF
} GfxStencilOperation;

typedef enum {
    GFX_SAMPLE_COUNT_1 = 1,
    GFX_SAMPLE_COUNT_2 = 2,
    GFX_SAMPLE_COUNT_4 = 4,
    GFX_SAMPLE_COUNT_8 = 8,
    GFX_SAMPLE_COUNT_16 = 16,
    GFX_SAMPLE_COUNT_32 = 32,
    GFX_SAMPLE_COUNT_64 = 64,
    GFX_SAMPLE_COUNT_MAX_ENUM = 0x7FFFFFFF
} GfxSampleCount;

typedef enum {
    GFX_SHADER_SOURCE_WGSL = 0, // WGSL text source (for WebGPU)
    GFX_SHADER_SOURCE_SPIRV = 1, // SPIR-V binary (for Vulkan)
    GFX_SHADER_SOURCE_MAX_ENUM = 0x7FFFFFFF
} GfxShaderSourceType;

// Result codes
typedef enum GfxResult {
    GFX_RESULT_SUCCESS = 0,

    // Operation-specific errors
    GFX_RESULT_TIMEOUT = 1,
    GFX_RESULT_NOT_READY = 2,

    // General errors
    GFX_RESULT_ERROR_INVALID_ARGUMENT = -1,
    GFX_RESULT_ERROR_NOT_FOUND = -2,
    GFX_RESULT_ERROR_OUT_OF_MEMORY = -3,
    GFX_RESULT_ERROR_DEVICE_LOST = -4,
    GFX_RESULT_ERROR_SURFACE_LOST = -5,
    GFX_RESULT_ERROR_OUT_OF_DATE = -6,
    // Backend errors
    GFX_RESULT_ERROR_BACKEND_NOT_LOADED = -7,
    GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED = -8,

    // Unknown/generic
    GFX_RESULT_ERROR_UNKNOWN = -9,
    GFX_RESULT_MAX_ENUM = 0x7FFFFFFF
} GfxResult;

typedef enum {
    GFX_LOAD_OP_LOAD = 0, // Load existing contents
    GFX_LOAD_OP_CLEAR = 1, // Clear to specified clear value
    GFX_LOAD_OP_DONT_CARE = 2, // Don't care about initial contents (better performance on tiled GPUs)
    GFX_LOAD_OP_MAX_ENUM = 0x7FFFFFFF
} GfxLoadOp;

typedef enum {
    GFX_STORE_OP_STORE = 0, // Store contents after render pass
    GFX_STORE_OP_DONT_CARE = 1, // Don't care about contents after render pass (better performance for transient attachments)
    GFX_STORE_OP_MAX_ENUM = 0x7FFFFFFF
} GfxStoreOp;

// Color write mask flags (can be combined with bitwise OR)
typedef enum {
    GFX_COLOR_WRITE_MASK_NONE = 0x0,
    GFX_COLOR_WRITE_MASK_RED = 0x1,
    GFX_COLOR_WRITE_MASK_GREEN = 0x2,
    GFX_COLOR_WRITE_MASK_BLUE = 0x4,
    GFX_COLOR_WRITE_MASK_ALPHA = 0x8,
    GFX_COLOR_WRITE_MASK_ALL = GFX_COLOR_WRITE_MASK_RED | GFX_COLOR_WRITE_MASK_GREEN | GFX_COLOR_WRITE_MASK_BLUE | GFX_COLOR_WRITE_MASK_ALPHA,
    GFX_COLOR_WRITE_MASK_MAX_ENUM = 0x7FFFFFFF
} GfxColorWriteMaskBits;
typedef uint32_t GfxColorWriteMask;

typedef enum {
    GFX_BIND_GROUP_ENTRY_TYPE_BUFFER = 0,
    GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER = 1,
    GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW = 2,
    GFX_BIND_GROUP_ENTRY_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxBindGroupEntryType;

typedef enum {
    GFX_BINDING_TYPE_BUFFER = 0,
    GFX_BINDING_TYPE_SAMPLER = 1,
    GFX_BINDING_TYPE_TEXTURE = 2,
    GFX_BINDING_TYPE_STORAGE_TEXTURE = 3,
    GFX_BINDING_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxBindingType;

typedef enum {
    GFX_FENCE_STATUS_UNSIGNALED = 0,
    GFX_FENCE_STATUS_SIGNALED = 1,
    GFX_FENCE_STATUS_ERROR = 2,
    GFX_FENCE_STATUS_MAX_ENUM = 0x7FFFFFFF
} GfxFenceStatus;

typedef enum {
    GFX_SEMAPHORE_TYPE_BINARY = 0,
    GFX_SEMAPHORE_TYPE_TIMELINE = 1,
    GFX_SEMAPHORE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxSemaphoreType;

typedef enum {
    GFX_QUERY_TYPE_OCCLUSION = 0,
    GFX_QUERY_TYPE_TIMESTAMP = 1,
    GFX_QUERY_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxQueryType;

// Structure types for extensibility (Vulkan-style)
typedef enum {
    GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR = 1,
    GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR = 2,
    GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR = 3,
    GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR = 4,
    GFX_STRUCTURE_TYPE_BUFFER_IMPORT_DESCRIPTOR = 5,
    GFX_STRUCTURE_TYPE_TEXTURE_DESCRIPTOR = 6,
    GFX_STRUCTURE_TYPE_TEXTURE_IMPORT_DESCRIPTOR = 7,
    GFX_STRUCTURE_TYPE_TEXTURE_VIEW_DESCRIPTOR = 8,
    GFX_STRUCTURE_TYPE_SAMPLER_DESCRIPTOR = 9,
    GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR = 10,
    GFX_STRUCTURE_TYPE_RENDER_PIPELINE_DESCRIPTOR = 11,
    GFX_STRUCTURE_TYPE_COMPUTE_PIPELINE_DESCRIPTOR = 12,
    GFX_STRUCTURE_TYPE_BIND_GROUP_LAYOUT_DESCRIPTOR = 13,
    GFX_STRUCTURE_TYPE_BIND_GROUP_DESCRIPTOR = 14,
    GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR = 15,
    GFX_STRUCTURE_TYPE_FRAMEBUFFER_DESCRIPTOR = 16,
    GFX_STRUCTURE_TYPE_FENCE_DESCRIPTOR = 17,
    GFX_STRUCTURE_TYPE_SEMAPHORE_DESCRIPTOR = 18,
    GFX_STRUCTURE_TYPE_QUERY_SET_DESCRIPTOR = 19,
    GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR = 20,
    GFX_STRUCTURE_TYPE_SURFACE_DESCRIPTOR = 21,
    GFX_STRUCTURE_TYPE_SWAPCHAIN_DESCRIPTOR = 22,
    GFX_STRUCTURE_TYPE_PIPELINE_BARRIER_DESCRIPTOR = 23,
    GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR = 24,
    GFX_STRUCTURE_TYPE_RENDER_PASS_BEGIN_DESCRIPTOR = 25,
    GFX_STRUCTURE_TYPE_COMPUTE_PASS_BEGIN_DESCRIPTOR = 26,
    GFX_STRUCTURE_TYPE_PRESENT_DESCRIPTOR = 27,
    GFX_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_INFO = 28,
    GFX_STRUCTURE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxStructureType;

// ============================================================================
// Extension Chain Base Structure
// ============================================================================

// Base header for all structures in pNext chains
// All extensible structures must start with these two fields
typedef struct GfxChainHeader {
    GfxStructureType sType;
    const void* pNext;
} GfxChainHeader;

// ============================================================================
// Extension Names (String Constants)
// ============================================================================

// Instance extensions
#define GFX_INSTANCE_EXTENSION_SURFACE "gfx_surface"
#define GFX_INSTANCE_EXTENSION_DEBUG "gfx_debug"

// Device extensions
#define GFX_DEVICE_EXTENSION_SWAPCHAIN "gfx_swapchain"
#define GFX_DEVICE_EXTENSION_TIMELINE_SEMAPHORE "gfx_timeline_semaphore"
#define GFX_DEVICE_EXTENSION_MULTIVIEW "gfx_multiview"
#define GFX_DEVICE_EXTENSION_ANISOTROPIC_FILTERING "gfx_anisotropic_filtering"

// ============================================================================
// Forward Declarations (Opaque Handles)
// ============================================================================

typedef struct GfxInstance_T* GfxInstance;
typedef struct GfxAdapter_T* GfxAdapter;
typedef struct GfxDevice_T* GfxDevice;
typedef struct GfxQueue_T* GfxQueue;
typedef struct GfxBuffer_T* GfxBuffer;
typedef struct GfxTexture_T* GfxTexture;
typedef struct GfxTextureView_T* GfxTextureView;
typedef struct GfxSampler_T* GfxSampler;
typedef struct GfxShader_T* GfxShader;
typedef struct GfxRenderPipeline_T* GfxRenderPipeline;
typedef struct GfxComputePipeline_T* GfxComputePipeline;
typedef struct GfxCommandEncoder_T* GfxCommandEncoder;
typedef struct GfxRenderPassEncoder_T* GfxRenderPassEncoder;
typedef struct GfxComputePassEncoder_T* GfxComputePassEncoder;
typedef struct GfxBindGroup_T* GfxBindGroup;
typedef struct GfxBindGroupLayout_T* GfxBindGroupLayout;
typedef struct GfxSurface_T* GfxSurface;
typedef struct GfxSwapchain_T* GfxSwapchain;
typedef struct GfxFence_T* GfxFence;
typedef struct GfxSemaphore_T* GfxSemaphore;
typedef struct GfxRenderPass_T* GfxRenderPass;
typedef struct GfxFramebuffer_T* GfxFramebuffer;
typedef struct GfxQuerySet_T* GfxQuerySet;

// ============================================================================
// Callback Function Types
// ============================================================================

typedef void (*GfxLogCallback)(GfxLogLevel level, const char* message, void* userData);

// ============================================================================
// Core Structures
// ============================================================================

typedef struct {
    float r;
    float g;
    float b;
    float a;
} GfxColor;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} GfxExtent3D;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} GfxOrigin3D;

typedef struct {
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
} GfxViewport;

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} GfxScissorRect;

typedef struct {
    GfxPipelineStageFlags srcStageMask;
    GfxPipelineStageFlags dstStageMask;
    GfxAccessFlags srcAccessMask;
    GfxAccessFlags dstAccessMask;
} GfxMemoryBarrier;

typedef struct {
    GfxBuffer buffer;
    GfxPipelineStageFlags srcStageMask;
    GfxPipelineStageFlags dstStageMask;
    GfxAccessFlags srcAccessMask;
    GfxAccessFlags dstAccessMask;
    uint64_t offset;
    uint64_t size;
} GfxBufferBarrier;

typedef struct {
    GfxTexture texture;
    GfxTextureLayout oldLayout;
    GfxTextureLayout newLayout;
    GfxPipelineStageFlags srcStageMask;
    GfxPipelineStageFlags dstStageMask;
    GfxAccessFlags srcAccessMask;
    GfxAccessFlags dstAccessMask;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
} GfxTextureBarrier;

// Simple load/store operations (clear values specified separately)
typedef struct {
    GfxLoadOp loadOp;
    GfxStoreOp storeOp;
} GfxLoadStoreOps;

// Color attachment target for render pass (main or resolve)
typedef struct {
    GfxTextureFormat format;
    GfxSampleCount sampleCount;
    GfxLoadStoreOps ops;
    GfxTextureLayout finalLayout;
} GfxRenderPassColorAttachmentTarget;

// Color attachment description for render pass
typedef struct {
    GfxRenderPassColorAttachmentTarget target;
    const GfxRenderPassColorAttachmentTarget* resolveTarget; // NULL if no resolve needed
} GfxRenderPassColorAttachment;

// Depth/stencil attachment target for render pass (main or resolve)
typedef struct {
    GfxTextureFormat format;
    GfxSampleCount sampleCount;
    GfxLoadStoreOps depthOps;
    GfxLoadStoreOps stencilOps;
    GfxTextureLayout finalLayout;
} GfxRenderPassDepthStencilAttachmentTarget;

// Depth/stencil attachment description for render pass
typedef struct {
    GfxRenderPassDepthStencilAttachmentTarget target;
    const GfxRenderPassDepthStencilAttachmentTarget* resolveTarget; // NULL if no resolve needed
} GfxRenderPassDepthStencilAttachment;

// Render pass descriptor: defines attachment formats and load/store operations (cached, reusable)
typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;

    // Color attachments
    const GfxRenderPassColorAttachment* colorAttachments;
    uint32_t colorAttachmentCount;

    // Depth/stencil attachment (optional)
    const GfxRenderPassDepthStencilAttachment* depthStencilAttachment; // NULL if not used
} GfxRenderPassDescriptor;

// Multiview rendering extension - allows rendering to multiple views in a single render pass
// Chain this to GfxRenderPassDescriptor.pNext to enable multiview
// Requires GFX_DEVICE_EXTENSION_MULTIVIEW to be enabled on device
// Example: For stereo rendering, set viewMask = 0x3 (views 0 and 1)
typedef struct {
    GfxStructureType sType;  // Must be GFX_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_INFO
    const void* pNext;
    
    // View mask - bit N set means view N is rendered
    // Example: 0x3 (binary 11) = render to view 0 and view 1 (stereo)
    // Example: 0x7 (binary 111) = render to views 0, 1, and 2
    uint32_t viewMask;
    
    // Correlation masks - groups of views that can be optimized together
    // Views in the same correlation mask share similar geometry/data
    // Example: For stereo, both eyes correlate: correlationMasks[0] = 0x3
    const uint32_t* correlationMasks;
    uint32_t correlationMaskCount;
} GfxRenderPassMultiviewInfo;

// Framebuffer attachment with optional resolve target
typedef struct {
    GfxTextureView view;
    GfxTextureView resolveTarget; // NULL if no resolve needed
} GfxFramebufferAttachment;

// Framebuffer descriptor: binds actual image views to a render pass
typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxRenderPass renderPass; // The render pass this framebuffer is compatible with

    // Color attachments with optional resolve targets
    const GfxFramebufferAttachment* colorAttachments;
    uint32_t colorAttachmentCount;

    // Depth/stencil attachment with optional resolve target (use {NULL, NULL} if not used)
    GfxFramebufferAttachment depthStencilAttachment;

    uint32_t width; // Framebuffer width
    uint32_t height; // Framebuffer height
} GfxFramebufferDescriptor;

// Render pass begin descriptor: used to BEGIN a render pass (with clear values)
typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxRenderPass renderPass; // The render pass to begin
    GfxFramebuffer framebuffer; // The framebuffer to render into

    // Clear values (per-frame data)
    const GfxColor* colorClearValues; // Array of clear colors (used when loadOp = CLEAR)
    uint32_t colorClearValueCount; // Number of color clear values (should match render pass color attachment count)
    float depthClearValue; // Used when depth loadOp = CLEAR
    uint32_t stencilClearValue; // Used when stencil loadOp = CLEAR
} GfxRenderPassBeginDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
} GfxComputePassBeginDescriptor;

// Copy/Blit descriptors
typedef struct {
    GfxBuffer source;
    uint64_t sourceOffset;
    GfxBuffer destination;
    uint64_t destinationOffset;
    uint64_t size;
} GfxCopyBufferToBufferDescriptor;

typedef struct {
    GfxBuffer source;
    uint64_t sourceOffset;
    GfxTexture destination;
    GfxOrigin3D origin;
    GfxExtent3D extent;
    uint32_t mipLevel;
    GfxTextureLayout finalLayout;
} GfxCopyBufferToTextureDescriptor;

typedef struct {
    GfxTexture source;
    GfxOrigin3D origin;
    uint32_t mipLevel;
    GfxBuffer destination;
    uint64_t destinationOffset;
    GfxExtent3D extent;
    GfxTextureLayout finalLayout;
} GfxCopyTextureToBufferDescriptor;

typedef struct {
    GfxTexture source;
    GfxOrigin3D sourceOrigin;
    uint32_t sourceMipLevel;
    GfxTextureLayout sourceFinalLayout;
    GfxTexture destination;
    GfxOrigin3D destinationOrigin;
    uint32_t destinationMipLevel;
    GfxTextureLayout destinationFinalLayout;
    GfxExtent3D extent;
} GfxCopyTextureToTextureDescriptor;

typedef struct {
    GfxTexture source;
    GfxOrigin3D sourceOrigin;
    GfxExtent3D sourceExtent;
    uint32_t sourceMipLevel;
    GfxTextureLayout sourceFinalLayout;
    GfxTexture destination;
    GfxOrigin3D destinationOrigin;
    GfxExtent3D destinationExtent;
    uint32_t destinationMipLevel;
    GfxTextureLayout destinationFinalLayout;
    GfxFilterMode filter;
} GfxBlitTextureToTextureDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const GfxMemoryBarrier* memoryBarriers;
    uint32_t memoryBarrierCount;
    const GfxBufferBarrier* bufferBarriers;
    uint32_t bufferBarrierCount;
    const GfxTextureBarrier* textureBarriers;
    uint32_t textureBarrierCount;
} GfxPipelineBarrierDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    GfxBackend backend;
    const char* applicationName;
    uint32_t applicationVersion;
    const char** enabledExtensions;
    uint32_t enabledExtensionCount;
} GfxInstanceDescriptor;

// Adapter selection: specify either an index OR a preference
// Set adapterIndex to UINT32_MAX to use preference-based selection
typedef struct {
    GfxStructureType sType;
    const void* pNext;
    uint32_t adapterIndex; // Adapter index from enumeration (use UINT32_MAX to ignore)
    GfxAdapterPreference preference; // Used only when adapterIndex is UINT32_MAX
} GfxAdapterDescriptor;

// Adapter information
typedef struct {
    const char* name;
    const char* driverDescription;
    uint32_t vendorID;
    uint32_t deviceID;
    GfxAdapterType adapterType;
    GfxBackend backend;
} GfxAdapterInfo;

// Texture information
typedef struct {
    GfxTextureType type;
    GfxExtent3D size;
    uint32_t arrayLayerCount;
    uint32_t mipLevelCount;
    GfxSampleCount sampleCount;
    GfxTextureFormat format;
    GfxTextureUsageFlags usage;
} GfxTextureInfo;

// Buffer information
typedef struct {
    uint64_t size;
    GfxBufferUsageFlags usage;
    GfxMemoryPropertyFlags memoryProperties;
} GfxBufferInfo;

// Swapchain information
typedef struct {
    uint32_t width;
    uint32_t height;
    GfxTextureFormat format;
    uint32_t imageCount;
    GfxPresentMode presentMode;
} GfxSwapchainInfo;

// Device limits
typedef struct {
    uint32_t minUniformBufferOffsetAlignment;
    uint32_t minStorageBufferOffsetAlignment;
    uint32_t maxUniformBufferBindingSize;
    uint32_t maxStorageBufferBindingSize;
    uint64_t maxBufferSize;
    uint32_t maxTextureDimension1D;
    uint32_t maxTextureDimension2D;
    uint32_t maxTextureDimension3D;
    uint32_t maxTextureArrayLayers;
} GfxDeviceLimits;

// Queue family properties
typedef struct {
    GfxQueueFlags flags;
    uint32_t queueCount;
} GfxQueueFamilyProperties;

// Queue request for device creation
typedef struct {
    uint32_t queueFamilyIndex;
    uint32_t queueIndex;
    float priority;
} GfxQueueRequest;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    const GfxQueueRequest* queueRequests; // Optional: explicit queue requests (NULL for automatic default queue)
    uint32_t queueRequestCount; // Number of queue requests (0 if queueRequests is NULL)
    const char** enabledExtensions;
    uint32_t enabledExtensionCount;
} GfxDeviceDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    uint64_t size;
    GfxBufferUsageFlags usage;
    GfxMemoryPropertyFlags memoryProperties;
} GfxBufferDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    void* nativeHandle; // VkBuffer or WGPUBuffer (cast to void*)
    uint64_t size;
    GfxBufferUsageFlags usage;
} GfxBufferImportDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxTextureType type;
    GfxExtent3D size;
    uint32_t arrayLayerCount;
    uint32_t mipLevelCount;
    GfxSampleCount sampleCount;
    GfxTextureFormat format;
    GfxTextureUsageFlags usage;
} GfxTextureDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    void* nativeHandle; // VkImage or WGPUTexture (cast to void*)
    GfxTextureType type;
    GfxExtent3D size;
    uint32_t arrayLayerCount;
    uint32_t mipLevelCount;
    GfxSampleCount sampleCount;
    GfxTextureFormat format;
    GfxTextureUsageFlags usage;
    GfxTextureLayout currentLayout; // Current layout of the imported texture
} GfxTextureImportDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxTextureViewType viewType;
    GfxTextureFormat format;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
} GfxTextureViewDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxAddressMode addressModeU;
    GfxAddressMode addressModeV;
    GfxAddressMode addressModeW;
    GfxFilterMode magFilter;
    GfxFilterMode minFilter;
    GfxFilterMode mipmapFilter;
    float lodMinClamp;
    float lodMaxClamp;
    GfxCompareFunction compare; // GFX_COMPARE_FUNCTION_UNDEFINED if not used
    uint16_t maxAnisotropy;
} GfxSamplerDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxShaderSourceType sourceType; // Explicitly specify WGSL or SPIR-V
    const void* code; // Shader code - WGSL source (const char*) or SPIR-V binary (uint32_t*)
    size_t codeSize; // Size in bytes - for SPIR-V: byte count, for WGSL: strlen or 0 for null-terminated
    const char* entryPoint;
} GfxShaderDescriptor;

typedef struct {
    GfxBlendOperation operation;
    GfxBlendFactor srcFactor;
    GfxBlendFactor dstFactor;
} GfxBlendComponent;

typedef struct {
    GfxBlendComponent color;
    GfxBlendComponent alpha;
} GfxBlendState;

typedef struct {
    GfxTextureFormat format;
    const GfxBlendState* blend; // NULL if not used
    GfxColorWriteMask writeMask; // Combination of GfxColorWriteMask flags
} GfxColorTargetState;

typedef struct {
    GfxTextureFormat format;
    uint64_t offset;
    uint32_t shaderLocation;
} GfxVertexAttribute;

typedef struct {
    uint64_t arrayStride;
    const GfxVertexAttribute* attributes;
    uint32_t attributeCount;
    GfxVertexStepMode stepMode;
} GfxVertexBufferLayout;

typedef struct {
    GfxShader module;
    const char* entryPoint;
    const GfxVertexBufferLayout* buffers;
    uint32_t bufferCount;
} GfxVertexState;

typedef struct {
    GfxShader module;
    const char* entryPoint;
    const GfxColorTargetState* targets;
    uint32_t targetCount;
} GfxFragmentState;

typedef struct {
    GfxPrimitiveTopology topology;
    GfxIndexFormat stripIndexFormat; // GFX_INDEX_FORMAT_UNDEFINED if not used
    GfxFrontFace frontFace;
    GfxCullMode cullMode;
    GfxPolygonMode polygonMode;
} GfxPrimitiveState;

typedef struct {
    GfxCompareFunction compare;
    GfxStencilOperation failOp;
    GfxStencilOperation depthFailOp;
    GfxStencilOperation passOp;
} GfxStencilFaceState;

typedef struct {
    GfxTextureFormat format;
    bool depthWriteEnabled;
    GfxCompareFunction depthCompare;
    GfxStencilFaceState stencilFront;
    GfxStencilFaceState stencilBack;
    uint32_t stencilReadMask;
    uint32_t stencilWriteMask;
    int32_t depthBias;
    float depthBiasSlopeScale;
    float depthBiasClamp;
} GfxDepthStencilState;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxRenderPass renderPass; // Render pass this pipeline will be used with
    const GfxVertexState* vertex;
    const GfxFragmentState* fragment; // NULL if not used
    const GfxPrimitiveState* primitive;
    const GfxDepthStencilState* depthStencil; // NULL if not used
    GfxSampleCount sampleCount;
    // Bind group layouts for the pipeline
    const GfxBindGroupLayout* bindGroupLayouts;
    uint32_t bindGroupLayoutCount;
} GfxRenderPipelineDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxShader compute;
    const char* entryPoint;
    // Bind group layouts for the pipeline
    const GfxBindGroupLayout* bindGroupLayouts;
    uint32_t bindGroupLayoutCount;
} GfxComputePipelineDescriptor;

typedef struct {
    uint32_t binding;
    GfxShaderStageFlags visibility;
    GfxBindingType type; // Explicitly specify the binding type

    // Resource type - use type field to determine which is valid
    struct {
        bool hasDynamicOffset;
        uint64_t minBindingSize;
    } buffer;

    struct {
        bool comparison;
    } sampler;

    struct {
        GfxTextureSampleType sampleType;
        GfxTextureViewType viewDimension;
        bool multisampled;
    } texture;

    struct {
        GfxTextureFormat format;
        GfxTextureViewType viewDimension;
        bool writeOnly;
    } storageTexture;
} GfxBindGroupLayoutEntry;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    const GfxBindGroupLayoutEntry* entries;
    uint32_t entryCount;
} GfxBindGroupLayoutDescriptor;

typedef struct {
    uint32_t binding;
    GfxBindGroupEntryType type;
    union {
        struct {
            GfxBuffer buffer;
            uint64_t offset;
            uint64_t size;
        } buffer;
        GfxSampler sampler;
        GfxTextureView textureView;
    } resource;
} GfxBindGroupEntry;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxBindGroupLayout layout;
    const GfxBindGroupEntry* entries;
    uint32_t entryCount;
} GfxBindGroupDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    bool signaled; // Initial state - true for signaled, false for unsignaled
} GfxFenceDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxSemaphoreType type;
    uint64_t initialValue; // For timeline semaphores, ignored for binary
} GfxSemaphoreDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxQueryType type;
    uint32_t count; // Number of queries in the set
} GfxQuerySetDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
} GfxCommandEncoderDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    GfxCommandEncoder* commandEncoders;
    uint32_t commandEncoderCount;

    // Wait semaphores (must be signaled before execution)
    GfxSemaphore* waitSemaphores;
    uint64_t* waitValues; // For timeline semaphores, NULL for binary
    uint32_t waitSemaphoreCount;

    // Signal semaphores (will be signaled after execution)
    GfxSemaphore* signalSemaphores;
    uint64_t* signalValues; // For timeline semaphores, NULL for binary
    uint32_t signalSemaphoreCount;

    // Optional fence to signal when all commands complete
    GfxFence signalFence;
} GfxSubmitDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    // Wait semaphores (rendering must complete before present)
    GfxSemaphore* waitSemaphores;
    uint32_t waitSemaphoreCount;
} GfxPresentDescriptor;

// ============================================================================
// Platform Specific
// ============================================================================

// Common windowing system enum for all platforms
typedef enum {
    GFX_WINDOWING_SYSTEM_WIN32 = 0,
    GFX_WINDOWING_SYSTEM_XLIB = 1,
    GFX_WINDOWING_SYSTEM_WAYLAND = 2,
    GFX_WINDOWING_SYSTEM_XCB = 3,
    GFX_WINDOWING_SYSTEM_METAL = 4,
    GFX_WINDOWING_SYSTEM_EMSCRIPTEN = 5,
    GFX_WINDOWING_SYSTEM_ANDROID = 6,
    GFX_WINDOWING_SYSTEM_MAX_ENUM = 0x7FFFFFFF
} GfxWindowingSystem;

// Common platform window handle struct with union for all windowing systems
typedef struct GfxWin32Handle {
    void* hwnd; // HWND - Window handle
    void* hinstance; // HINSTANCE - Application instance
} GfxWin32Handle;

typedef struct GfxXlibHandle {
    void* display; // Display*
    unsigned long window; // Window
} GfxXlibHandle;

typedef struct GfxWaylandHandle {
    void* surface; // wl_surface*
    void* display; // wl_display*
} GfxWaylandHandle;

typedef struct GfxXcbHandle {
    void* connection; // xcb_connection_t*
    uint32_t window; // xcb_window_t
} GfxXcbHandle;

typedef struct GfxMetalHandle {
    void* layer; // CAMetalLayer* (optional)
} GfxMetalHandle;

typedef struct GfxEmscriptenHandle {
    const char* canvasSelector; // CSS selector for canvas element (e.g., "#canvas")
} GfxEmscriptenHandle;

typedef struct GfxAndroidHandle {
    void* window;
} GfxAndroidHandle;

typedef struct {
    GfxWindowingSystem windowingSystem;
    union {
        GfxWin32Handle win32;
        GfxXlibHandle xlib;
        GfxXcbHandle xcb;
        GfxWaylandHandle wayland;
        GfxMetalHandle metal;
        GfxEmscriptenHandle emscripten;
        GfxAndroidHandle android;
    };
} GfxPlatformWindowHandle;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxPlatformWindowHandle windowHandle;
} GfxSurfaceDescriptor;

typedef struct {
    GfxStructureType sType;
    const void* pNext;
    const char* label;
    GfxSurface surface;
    uint32_t width;
    uint32_t height;
    GfxTextureFormat format;
    GfxTextureUsageFlags usage;
    GfxPresentMode presentMode;
    uint32_t imageCount;
} GfxSwapchainDescriptor;

// ============================================================================
// API Functions
// ============================================================================
//
// PARAMETER CONVENTIONS:
// - Input-only descriptor pointers are marked 'const'
// - Output parameters use 'out' prefix (e.g., outInstance, outAdapter)
// - Boolean output parameters use 'is' or 'outSupported' prefix
// - Array + count parameters follow Vulkan style: pass NULL to query count
//
// THREAD SAFETY:
// - Object creation/destruction functions are NOT thread-safe
// - Multiple threads can submit to different queues simultaneously
// - Single queue must not be accessed from multiple threads concurrently
// - Command encoder recording is NOT thread-safe (one per thread)
//

// Backend loading/unloading functions
// These should be called before creating any instances
// Call gfxLoadBackend or gfxLoadAllBackends at application startup
// Call gfxUnloadBackend or gfxUnloadAllBackends at application shutdown
GFX_API GfxResult gfxLoadBackend(GfxBackend backend);
GFX_API GfxResult gfxUnloadBackend(GfxBackend backend);
GFX_API GfxResult gfxLoadAllBackends(void);
GFX_API GfxResult gfxUnloadAllBackends(void);

// Version query function - Returns the runtime library version
// This allows checking if the compiled version matches the linked version
GFX_API GfxResult gfxGetVersion(uint32_t* major, uint32_t* minor, uint32_t* patch);

// Extension enumeration functions
// Vulkan-style enumeration: call with queueFamilies=NULL to get count, then call again with allocated array
GFX_API GfxResult gfxEnumerateInstanceExtensions(GfxBackend backend, uint32_t* extensionCount, const char** extensionNames);

// Instance functions
GFX_API GfxResult gfxCreateInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance);
GFX_API GfxResult gfxInstanceDestroy(GfxInstance instance);
GFX_API GfxResult gfxInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter);
// Vulkan-style enumeration: call with adapters=NULL to get count, then call again with allocated array
GFX_API GfxResult gfxInstanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters);

// Adapter functions
GFX_API GfxResult gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice);
GFX_API GfxResult gfxAdapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo);
GFX_API GfxResult gfxAdapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits);
// Vulkan-style enumeration: call with queueFamilies=NULL to get count, then call again with allocated array
GFX_API GfxResult gfxAdapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies);
GFX_API GfxResult gfxAdapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported);
// Vulkan-style enumeration: call with queueFamilies=NULL to get count, then call again with allocated array
GFX_API GfxResult gfxAdapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames);

// Device functions
GFX_API GfxResult gfxDeviceDestroy(GfxDevice device);
GFX_API GfxResult gfxDeviceGetQueue(GfxDevice device, GfxQueue* outQueue);
GFX_API GfxResult gfxDeviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue);
GFX_API GfxResult gfxDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface);
GFX_API GfxResult gfxDeviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain);
GFX_API GfxResult gfxDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer);
GFX_API GfxResult gfxDeviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer);
GFX_API GfxResult gfxDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture);
GFX_API GfxResult gfxDeviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture);
GFX_API GfxResult gfxDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler);
GFX_API GfxResult gfxDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader);
GFX_API GfxResult gfxDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout);
GFX_API GfxResult gfxDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup);
GFX_API GfxResult gfxDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline);
GFX_API GfxResult gfxDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline);
GFX_API GfxResult gfxDeviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder);
GFX_API GfxResult gfxDeviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass);
GFX_API GfxResult gfxDeviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer);
GFX_API GfxResult gfxDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence);
GFX_API GfxResult gfxDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore);
GFX_API GfxResult gfxDeviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet);
GFX_API GfxResult gfxDeviceWaitIdle(GfxDevice device);
GFX_API GfxResult gfxDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits);
GFX_API GfxResult gfxDeviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported);

// Surface functions
GFX_API GfxResult gfxSurfaceDestroy(GfxSurface surface);
// Vulkan-style enumeration: call with formats=NULL to get count, then call again with allocated array
GFX_API GfxResult gfxSurfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats);
// Vulkan-style enumeration: call with presentModes=NULL to get count, then call again with allocated array
GFX_API GfxResult gfxSurfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes);

// Swapchain functions
GFX_API GfxResult gfxSwapchainDestroy(GfxSwapchain swapchain);
GFX_API GfxResult gfxSwapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo);
GFX_API GfxResult gfxSwapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex);
GFX_API GfxResult gfxSwapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView);
GFX_API GfxResult gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView);
GFX_API GfxResult gfxSwapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor);

// Buffer functions
GFX_API GfxResult gfxBufferDestroy(GfxBuffer buffer);
GFX_API GfxResult gfxBufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo);
GFX_API GfxResult gfxBufferGetNativeHandle(GfxBuffer buffer, void** outHandle);
GFX_API GfxResult gfxBufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer);
GFX_API GfxResult gfxBufferUnmap(GfxBuffer buffer);
GFX_API GfxResult gfxBufferFlushMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size);
GFX_API GfxResult gfxBufferInvalidateMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size);

// Texture functions
GFX_API GfxResult gfxTextureDestroy(GfxTexture texture);
GFX_API GfxResult gfxTextureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo);
GFX_API GfxResult gfxTextureGetNativeHandle(GfxTexture texture, void** outHandle);
GFX_API GfxResult gfxTextureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout);
GFX_API GfxResult gfxTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView);

// TextureView functions
GFX_API GfxResult gfxTextureViewDestroy(GfxTextureView textureView);

// Sampler functions
GFX_API GfxResult gfxSamplerDestroy(GfxSampler sampler);

// Shader functions
GFX_API GfxResult gfxShaderDestroy(GfxShader shader);

// BindGroupLayout functions
GFX_API GfxResult gfxBindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout);

// BindGroup functions
GFX_API GfxResult gfxBindGroupDestroy(GfxBindGroup bindGroup);

// RenderPipeline functions
GFX_API GfxResult gfxRenderPipelineDestroy(GfxRenderPipeline renderPipeline);

// ComputePipeline functions
GFX_API GfxResult gfxComputePipelineDestroy(GfxComputePipeline computePipeline);

// RenderPass functions
GFX_API GfxResult gfxRenderPassDestroy(GfxRenderPass renderPass);

// Framebuffer functions
GFX_API GfxResult gfxFramebufferDestroy(GfxFramebuffer framebuffer);

// QuerySet functions
GFX_API GfxResult gfxQuerySetDestroy(GfxQuerySet querySet);

// Queue functions
GFX_API GfxResult gfxQueueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitDescriptor);
GFX_API GfxResult gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size);
GFX_API GfxResult gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout);
GFX_API GfxResult gfxQueueWaitIdle(GfxQueue queue);

// CommandEncoder functions
GFX_API GfxResult gfxCommandEncoderDestroy(GfxCommandEncoder commandEncoder);
GFX_API GfxResult gfxCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass);
GFX_API GfxResult gfxCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass);
GFX_API GfxResult gfxCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor);
GFX_API GfxResult gfxCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor);
GFX_API GfxResult gfxCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor);
GFX_API GfxResult gfxCommandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor);
GFX_API GfxResult gfxCommandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor);
GFX_API GfxResult gfxCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor);
GFX_API GfxResult gfxCommandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture);
GFX_API GfxResult gfxCommandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture, uint32_t baseMipLevel, uint32_t levelCount);
GFX_API GfxResult gfxCommandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex);
GFX_API GfxResult gfxCommandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset);
GFX_API GfxResult gfxCommandEncoderEnd(GfxCommandEncoder commandEncoder);
GFX_API GfxResult gfxCommandEncoderBegin(GfxCommandEncoder commandEncoder);

// RenderPassEncoder functions
GFX_API GfxResult gfxRenderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline);
GFX_API GfxResult gfxRenderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
GFX_API GfxResult gfxRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size);
GFX_API GfxResult gfxRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size);
GFX_API GfxResult gfxRenderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport);
GFX_API GfxResult gfxRenderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor);
GFX_API GfxResult gfxRenderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
GFX_API GfxResult gfxRenderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
GFX_API GfxResult gfxRenderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset);
GFX_API GfxResult gfxRenderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset);
GFX_API GfxResult gfxRenderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex);
GFX_API GfxResult gfxRenderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder);
GFX_API GfxResult gfxRenderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder);

// ComputePassEncoder functions
GFX_API GfxResult gfxComputePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline);
GFX_API GfxResult gfxComputePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
GFX_API GfxResult gfxComputePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
GFX_API GfxResult gfxComputePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset);
GFX_API GfxResult gfxComputePassEncoderEnd(GfxComputePassEncoder computePassEncoder);

// Fence functions
GFX_API GfxResult gfxFenceDestroy(GfxFence fence);
GFX_API GfxResult gfxFenceGetStatus(GfxFence fence, bool* isSignaled);
GFX_API GfxResult gfxFenceWait(GfxFence fence, uint64_t timeoutNs);
GFX_API GfxResult gfxFenceReset(GfxFence fence);

// Semaphore functions
GFX_API GfxResult gfxSemaphoreDestroy(GfxSemaphore semaphore);
GFX_API GfxResult gfxSemaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType);
GFX_API GfxResult gfxSemaphoreSignal(GfxSemaphore semaphore, uint64_t value);
GFX_API GfxResult gfxSemaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs);
GFX_API GfxResult gfxSemaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue);

// ============================================================================
// Utility Functions
// ============================================================================

// Logging function
// Set a callback to receive log messages from the library
// Pass NULL to callback to disable logging
GFX_API void gfxSetLogCallback(GfxLogCallback callback, void* userData);

// Error handling
// Convert a GfxResult code to a human-readable string
// Returns a static string that does not need to be freed
// Example: "GFX_RESULT_SUCCESS", "GFX_RESULT_ERROR_OUT_OF_MEMORY"
GFX_API const char* gfxResultToString(GfxResult result);

// Helper function to deduce access flags from texture layout
// Returns appropriate access flags for the given layout (deterministic mapping)
// Note: WebGPU backends with implicit synchronization may ignore these flags
GFX_API GfxAccessFlags gfxGetAccessFlagsForLayout(GfxTextureLayout layout);

// Alignment helper functions
// Use these to align buffer offsets/sizes to device requirements:
// Example: uint64_t alignedOffset = gfxAlignUp(offset, limits.minUniformBufferOffsetAlignment);
GFX_API uint64_t gfxAlignUp(uint64_t value, uint64_t alignment);
GFX_API uint64_t gfxAlignDown(uint64_t value, uint64_t alignment);

// Format helper functions
// Get the size in bytes of a single pixel/texel for a given format
GFX_API uint32_t gfxGetFormatBytesPerPixel(GfxTextureFormat format);

// Cross-platform helpers available on all platforms
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromXlib(void* display, unsigned long window);
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromWayland(void* surface, void* display);
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromXCB(void* connection, uint32_t window);
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromWin32(void* hwnd, void* hinstance);
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromEmscripten(const char* canvasSelector);
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromAndroid(void* window);
GFX_API GfxPlatformWindowHandle gfxPlatformWindowHandleFromMetal(void* layer);

#ifdef __cplusplus
}
#endif

#endif // GFX_GFX_H