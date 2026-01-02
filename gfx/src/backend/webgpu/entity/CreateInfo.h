#pragma once

#include <webgpu/webgpu.h>
#include <cstdint>

namespace gfx::webgpu {

// ============================================================================
// Internal Type Definitions
// ============================================================================

// Internal buffer usage flags (WebGPU native)
using BufferUsage = WGPUBufferUsage;

// Internal semaphore type
enum class SemaphoreType {
    Binary,
    Timeline
};

// Internal platform window handle
struct PlatformWindowHandle {
    enum class Platform {
        Unknown,
        XCB,
        Xlib,
        Wayland,
        Win32,
        MacOS,
        Android
    } platform;

    union {
        struct {
            void* connection;
            uint32_t window;
        } xcb;
        struct {
            void* display;
            unsigned long window;
        } xlib;
        struct {
            void* display;
            void* surface;
        } wayland;
        struct {
            void* hinstance;
            void* hwnd;
        } win32;
        struct {
            void* layer;
        } macos;
        struct {
            void* window;
        } android;
    } handle;
};

// ============================================================================
// Internal CreateInfo Structs
// ============================================================================

struct InstanceCreateInfo {
    bool enableValidation;
};

} // namespace gfx::webgpu
