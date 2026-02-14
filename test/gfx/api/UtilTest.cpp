#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Non-parameterized Tests - These are backend-independent utility functions
// ===========================================================================

// Alignment tests

namespace {

TEST(GfxUtilTest, AlignUpBasic)
{
    EXPECT_EQ(gfxAlignUp(0, 4), 0);
    EXPECT_EQ(gfxAlignUp(1, 4), 4);
    EXPECT_EQ(gfxAlignUp(4, 4), 4);
    EXPECT_EQ(gfxAlignUp(5, 4), 8);
    EXPECT_EQ(gfxAlignUp(8, 4), 8);
}

TEST(GfxUtilTest, AlignUpPowerOfTwo)
{
    EXPECT_EQ(gfxAlignUp(0, 256), 0);
    EXPECT_EQ(gfxAlignUp(1, 256), 256);
    EXPECT_EQ(gfxAlignUp(255, 256), 256);
    EXPECT_EQ(gfxAlignUp(256, 256), 256);
    EXPECT_EQ(gfxAlignUp(257, 256), 512);
}

TEST(GfxUtilTest, AlignUpLargeValues)
{
    EXPECT_EQ(gfxAlignUp(1000, 256), 1024);
    EXPECT_EQ(gfxAlignUp(1024, 256), 1024);
    EXPECT_EQ(gfxAlignUp(1025, 256), 1280);
}

TEST(GfxUtilTest, AlignDownBasic)
{
    EXPECT_EQ(gfxAlignDown(0, 4), 0);
    EXPECT_EQ(gfxAlignDown(1, 4), 0);
    EXPECT_EQ(gfxAlignDown(4, 4), 4);
    EXPECT_EQ(gfxAlignDown(5, 4), 4);
    EXPECT_EQ(gfxAlignDown(8, 4), 8);
}

TEST(GfxUtilTest, AlignDownPowerOfTwo)
{
    EXPECT_EQ(gfxAlignDown(0, 256), 0);
    EXPECT_EQ(gfxAlignDown(1, 256), 0);
    EXPECT_EQ(gfxAlignDown(255, 256), 0);
    EXPECT_EQ(gfxAlignDown(256, 256), 256);
    EXPECT_EQ(gfxAlignDown(257, 256), 256);
}

TEST(GfxUtilTest, AlignDownLargeValues)
{
    EXPECT_EQ(gfxAlignDown(1000, 256), 768);
    EXPECT_EQ(gfxAlignDown(1024, 256), 1024);
    EXPECT_EQ(gfxAlignDown(1025, 256), 1024);
}

// Format helper tests
TEST(GfxUtilTest, GetFormatBytesPerPixel8Bit)
{
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R8_UNORM), 1);
}

TEST(GfxUtilTest, GetFormatBytesPerPixel16Bit)
{
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R8G8_UNORM), 2);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R16_FLOAT), 2);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R16G16_FLOAT), 4);
}

TEST(GfxUtilTest, GetFormatBytesPerPixel32Bit)
{
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R32_FLOAT), 4);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM), 4);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB), 4);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM), 4);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB), 4);
}

TEST(GfxUtilTest, GetFormatBytesPerPixel64Bit)
{
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT), 8);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R32G32_FLOAT), 8);
}

TEST(GfxUtilTest, GetFormatBytesPerPixel128Bit)
{
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R32G32B32_FLOAT), 12);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT), 16);
}

TEST(GfxUtilTest, GetFormatBytesPerPixelDepthStencil)
{
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_DEPTH16_UNORM), 2);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_DEPTH32_FLOAT), 4);
    EXPECT_EQ(gfxGetFormatBytesPerPixel(GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8), 4);
}

// ===========================================================================
// Non-parameterized Tests - Backend-independent utility functions
// ===========================================================================

// Platform window handle creation tests
// These verify the functions set the type correctly and store the input values
TEST(GfxUtilTest, PlatformWindowHandleFromXlib)
{
    void* display = (void*)0x1234;
    unsigned long window = 5678;
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromXlib(display, window);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_XLIB);
    EXPECT_EQ(handle.xlib.display, display);
    EXPECT_EQ(handle.xlib.window, window);
}

TEST(GfxUtilTest, PlatformWindowHandleFromWayland)
{
    void* surface = (void*)0x1234;
    void* display = (void*)0x5678;
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromWayland(display, surface);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_WAYLAND);
    EXPECT_EQ(handle.wayland.surface, surface);
    EXPECT_EQ(handle.wayland.display, display);
}

TEST(GfxUtilTest, PlatformWindowHandleFromXCB)
{
    void* connection = (void*)0x1234;
    uint32_t window = 5678;
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromXCB(connection, window);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_XCB);
    EXPECT_EQ(handle.xcb.connection, connection);
    EXPECT_EQ(handle.xcb.window, window);
}

TEST(GfxUtilTest, PlatformWindowHandleFromWin32)
{
    void* hinstance = (void*)0x5678;
    void* hwnd = (void*)0x1234;
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromWin32(hinstance, hwnd);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_WIN32);
    EXPECT_EQ(handle.win32.hinstance, hinstance);
    EXPECT_EQ(handle.win32.hwnd, hwnd);
}

TEST(GfxUtilTest, PlatformWindowHandleFromEmscripten)
{
    const char* selector = "#canvas";
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromEmscripten(selector);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_EMSCRIPTEN);
    EXPECT_EQ(handle.emscripten.canvasSelector, selector);
}

TEST(GfxUtilTest, PlatformWindowHandleFromAndroid)
{
    void* window = (void*)0x1234;
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromAndroid(window);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_ANDROID);
    EXPECT_EQ(handle.android.window, window);
}

TEST(GfxUtilTest, PlatformWindowHandleFromMetal)
{
    void* window = nullptr;
    GfxPlatformWindowHandle handle = gfxPlatformWindowHandleFromMetal(window);
    EXPECT_EQ(handle.windowingSystem, GFX_WINDOWING_SYSTEM_METAL);
    // the layer is internally derived from window, so the input value is not stored directly.
    // The handle should be valid but the layer field is not expected to match the input.
    EXPECT_EQ(handle.metal.layer, nullptr);
}

} // namespace
