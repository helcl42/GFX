#ifndef GFX_VULKAN_COMMON_H
#define GFX_VULKAN_COMMON_H

#ifndef GFX_HEADLESS_BUILD
#ifdef GFX_HAS_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif

#ifdef GFX_HAS_X11
#define VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#endif

#ifdef GFX_HAS_XCB
#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#endif

#ifdef GFX_HAS_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

#ifdef GFX_HAS_COCOA
#define VK_USE_PLATFORM_MACOS_MVK
// Metal layer type needed for MoltenVK
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// Forward declare when compiling as C/C++
typedef void CAMetalLayer;
#endif
#endif

#ifdef GFX_HAS_UIKIT
#define VK_USE_PLATFORM_IOS_MVK
// Metal layer type needed for MoltenVK
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#else
// Forward declare when compiling as C/C++
typedef void CAMetalLayer;
#endif
#endif

#ifdef GFX_HAS_ANDROID
#define VK_USE_PLATFORM_ANDROID_KHR
#include <android/native_window.h>
#endif
#endif // GFX_HEADLESS_BUILD

#include <vulkan/vulkan.h>

#endif // VULKAN_COMMON_H