#ifndef GFX_WEBGPU_COMMON_H
#define GFX_WEBGPU_COMMON_H

// Platform-specific includes for surface creation
#ifndef GFX_HEADLESS_BUILD
#ifdef GFX_HAS_EMSCRIPTEN
#include <emscripten.h>
#endif
#ifdef GFX_HAS_WIN32
#include <windows.h>
#endif
#ifdef GFX_HAS_X11
#include <X11/Xlib.h>
#endif
#ifdef GFX_HAS_XCB
#include <xcb/xcb.h>
#endif
#ifdef GFX_HAS_WAYLAND
#include <wayland-client.h>
#endif
#ifdef GFX_HAS_COCOA
// Metal layer type needed for MoltenVK
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// Forward declare when compiling as C/C++
typedef void CAMetalLayer;
#endif
#endif
#ifdef GFX_HAS_UIKIT
// Metal layer type needed for MoltenVK
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#else
// Forward declare when compiling as C/C++
typedef void CAMetalLayer;
#endif
#endif
#ifdef GFX_HAS_ANDROID
#include <android/native_window.h>
#endif
#endif // GFX_HEADLESS_BUILD

#include <webgpu/webgpu.h>

// Undefine X11 macros that conflict with other headers (e.g., gtest)
// These must be undefined after including X11 headers
#ifdef GFX_HAS_X11
#undef None
#undef Bool
#undef Status
#undef Success
#undef True
#undef False
#endif

#endif // GFX_WEBGPU_COMMON_H
