#include "CommonTest.h"

// ===========================================================================
// Non-parameterized Tests - These are backend-independent utility functions
// ===========================================================================

// Alignment tests

namespace {

TEST(GfxCppUtilTest, AlignUpBasic)
{
    EXPECT_EQ(gfx::utils::alignUp(0, 4), 0);
    EXPECT_EQ(gfx::utils::alignUp(1, 4), 4);
    EXPECT_EQ(gfx::utils::alignUp(4, 4), 4);
    EXPECT_EQ(gfx::utils::alignUp(5, 4), 8);
    EXPECT_EQ(gfx::utils::alignUp(8, 4), 8);
}

TEST(GfxCppUtilTest, AlignUpPowerOfTwo)
{
    EXPECT_EQ(gfx::utils::alignUp(0, 256), 0);
    EXPECT_EQ(gfx::utils::alignUp(1, 256), 256);
    EXPECT_EQ(gfx::utils::alignUp(255, 256), 256);
    EXPECT_EQ(gfx::utils::alignUp(256, 256), 256);
    EXPECT_EQ(gfx::utils::alignUp(257, 256), 512);
}

TEST(GfxCppUtilTest, AlignUpLargeValues)
{
    EXPECT_EQ(gfx::utils::alignUp(1000, 256), 1024);
    EXPECT_EQ(gfx::utils::alignUp(1024, 256), 1024);
    EXPECT_EQ(gfx::utils::alignUp(1025, 256), 1280);
}

TEST(GfxCppUtilTest, AlignDownBasic)
{
    EXPECT_EQ(gfx::utils::alignDown(0, 4), 0);
    EXPECT_EQ(gfx::utils::alignDown(1, 4), 0);
    EXPECT_EQ(gfx::utils::alignDown(4, 4), 4);
    EXPECT_EQ(gfx::utils::alignDown(5, 4), 4);
    EXPECT_EQ(gfx::utils::alignDown(8, 4), 8);
}

TEST(GfxCppUtilTest, AlignDownPowerOfTwo)
{
    EXPECT_EQ(gfx::utils::alignDown(0, 256), 0);
    EXPECT_EQ(gfx::utils::alignDown(1, 256), 0);
    EXPECT_EQ(gfx::utils::alignDown(255, 256), 0);
    EXPECT_EQ(gfx::utils::alignDown(256, 256), 256);
    EXPECT_EQ(gfx::utils::alignDown(257, 256), 256);
}

TEST(GfxCppUtilTest, AlignDownLargeValues)
{
    EXPECT_EQ(gfx::utils::alignDown(1000, 256), 768);
    EXPECT_EQ(gfx::utils::alignDown(1024, 256), 1024);
    EXPECT_EQ(gfx::utils::alignDown(1025, 256), 1024);
}

// Format helper tests
TEST(GfxCppUtilTest, GetFormatBytesPerPixel8Bit)
{
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R8Unorm), 1);
}

TEST(GfxCppUtilTest, GetFormatBytesPerPixel16Bit)
{
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R8G8Unorm), 2);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R16Float), 2);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R16G16Float), 4);
}

TEST(GfxCppUtilTest, GetFormatBytesPerPixel32Bit)
{
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R32Float), 4);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R8G8B8A8Unorm), 4);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R8G8B8A8UnormSrgb), 4);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::B8G8R8A8Unorm), 4);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::B8G8R8A8UnormSrgb), 4);
}

TEST(GfxCppUtilTest, GetFormatBytesPerPixel64Bit)
{
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R16G16B16A16Float), 8);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R32G32Float), 8);
}

TEST(GfxCppUtilTest, GetFormatBytesPerPixel128Bit)
{
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R32G32B32Float), 12);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::R32G32B32A32Float), 16);
}

TEST(GfxCppUtilTest, GetFormatBytesPerPixelDepthStencil)
{
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::Depth16Unorm), 2);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::Depth32Float), 4);
    EXPECT_EQ(gfx::utils::getFormatBytesPerPixel(gfx::TextureFormat::Depth24PlusStencil8), 4);
}

// Platform window handle creation tests
// These verify the functions set the type correctly and store the input values
TEST(GfxCppUtilTest, PlatformWindowHandleFromXlib)
{
    void* display = (void*)0x1234;
    unsigned long window = 5678;
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromXlib(display, window);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::Xlib);
    EXPECT_EQ(handle.handle.xlib.display, display);
    EXPECT_EQ(handle.handle.xlib.window, window);
}

TEST(GfxCppUtilTest, PlatformWindowHandleFromWayland)
{
    void* surface = (void*)0x1234;
    void* display = (void*)0x5678;
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromWayland(display, surface);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::Wayland);
    EXPECT_EQ(handle.handle.wayland.surface, surface);
    EXPECT_EQ(handle.handle.wayland.display, display);
}

TEST(GfxCppUtilTest, PlatformWindowHandleFromXCB)
{
    void* connection = (void*)0x1234;
    uint32_t window = 5678;
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromXCB(connection, window);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::XCB);
    EXPECT_EQ(handle.handle.xcb.connection, connection);
    EXPECT_EQ(handle.handle.xcb.window, window);
}

TEST(GfxCppUtilTest, PlatformWindowHandleFromWin32)
{
    void* hwnd = (void*)0x1234;
    void* hinstance = (void*)0x5678;
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromWin32(hinstance, hwnd);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::Win32);
    EXPECT_EQ(handle.handle.win32.hwnd, hwnd);
    EXPECT_EQ(handle.handle.win32.hinstance, hinstance);
}

TEST(GfxCppUtilTest, PlatformWindowHandleFromEmscripten)
{
    const char* selector = "#canvas";
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromEmscripten(selector);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::Emscripten);
    EXPECT_EQ(handle.handle.emscripten.canvasSelector, selector);
}

TEST(GfxCppUtilTest, PlatformWindowHandleFromMetal)
{
    void* window = nullptr;
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromMetal(window);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::Metal);
    EXPECT_EQ(handle.handle.metal.layer, nullptr);
}

// ============================================================================
// Result to String Conversion Tests
// ============================================================================

TEST(GfxCppUtilTest, ResultToStringSuccess)
{
    const char* str = gfx::utils::resultToString(gfx::Result::Success);
    EXPECT_STREQ(str, "Result::Success");
}

TEST(GfxCppUtilTest, ResultToStringTimeout)
{
    const char* str = gfx::utils::resultToString(gfx::Result::Timeout);
    EXPECT_STREQ(str, "Result::Timeout");
}

TEST(GfxCppUtilTest, ResultToStringNotReady)
{
    const char* str = gfx::utils::resultToString(gfx::Result::NotReady);
    EXPECT_STREQ(str, "Result::NotReady");
}

TEST(GfxCppUtilTest, ResultToStringErrorInvalidArgument)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorInvalidArgument);
    EXPECT_STREQ(str, "Result::ErrorInvalidArgument");
}

TEST(GfxCppUtilTest, ResultToStringErrorNotFound)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorNotFound);
    EXPECT_STREQ(str, "Result::ErrorNotFound");
}

TEST(GfxCppUtilTest, ResultToStringErrorOutOfMemory)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorOutOfMemory);
    EXPECT_STREQ(str, "Result::ErrorOutOfMemory");
}

TEST(GfxCppUtilTest, ResultToStringErrorDeviceLost)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorDeviceLost);
    EXPECT_STREQ(str, "Result::ErrorDeviceLost");
}

TEST(GfxCppUtilTest, ResultToStringErrorSurfaceLost)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorSurfaceLost);
    EXPECT_STREQ(str, "Result::ErrorSurfaceLost");
}

TEST(GfxCppUtilTest, ResultToStringErrorOutOfDate)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorOutOfDate);
    EXPECT_STREQ(str, "Result::ErrorOutOfDate");
}

TEST(GfxCppUtilTest, ResultToStringErrorBackendNotLoaded)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorBackendNotLoaded);
    EXPECT_STREQ(str, "Result::ErrorBackendNotLoaded");
}

TEST(GfxCppUtilTest, ResultToStringErrorFeatureNotSupported)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorFeatureNotSupported);
    EXPECT_STREQ(str, "Result::ErrorFeatureNotSupported");
}

TEST(GfxCppUtilTest, ResultToStringErrorUnknown)
{
    const char* str = gfx::utils::resultToString(gfx::Result::ErrorUnknown);
    EXPECT_STREQ(str, "Result::ErrorUnknown");
}

// Test for unknown/invalid result values
TEST(GfxCppUtilTest, ResultToStringUnknownValue)
{
    // Cast an invalid value to Result
    const char* str = gfx::utils::resultToString(static_cast<gfx::Result>(999));
    EXPECT_STREQ(str, "Result::Unknown");
}

TEST(GfxCppUtilTest, ResultToStringAllValuesNonNull)
{
    // Verify all Result values produce non-null strings
    std::vector<gfx::Result> results = {
        gfx::Result::Success,
        gfx::Result::Timeout,
        gfx::Result::NotReady,
        gfx::Result::ErrorInvalidArgument,
        gfx::Result::ErrorNotFound,
        gfx::Result::ErrorOutOfMemory,
        gfx::Result::ErrorDeviceLost,
        gfx::Result::ErrorSurfaceLost,
        gfx::Result::ErrorOutOfDate,
        gfx::Result::ErrorBackendNotLoaded,
        gfx::Result::ErrorFeatureNotSupported,
        gfx::Result::ErrorUnknown
    };

    for (gfx::Result result : results) {
        const char* str = gfx::utils::resultToString(result);
        EXPECT_NE(str, nullptr);
        EXPECT_GT(std::strlen(str), 0); // Non-empty string
    }
}

TEST(GfxCppUtilTest, ResultToStringConsistent)
{
    // Verify that calling the function multiple times returns the same string
    const char* str1 = gfx::utils::resultToString(gfx::Result::ErrorOutOfMemory);
    const char* str2 = gfx::utils::resultToString(gfx::Result::ErrorOutOfMemory);
    EXPECT_STREQ(str1, str2);
    // They should point to the same static string
    EXPECT_EQ(str1, str2);
}

TEST(GfxCppUtilTest, ResultToStringErrorCodesAreNegative)
{
    // Verify that error Result values are negative
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorInvalidArgument), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorNotFound), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorOutOfMemory), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorDeviceLost), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorSurfaceLost), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorOutOfDate), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorBackendNotLoaded), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorFeatureNotSupported), 0);
    EXPECT_LT(static_cast<int>(gfx::Result::ErrorUnknown), 0);
}

TEST(GfxCppUtilTest, ResultToStringSuccessCodesAreNonNegative)
{
    // Verify that success Result values are non-negative
    EXPECT_GE(static_cast<int>(gfx::Result::Success), 0);
    EXPECT_GE(static_cast<int>(gfx::Result::Timeout), 0);
    EXPECT_GE(static_cast<int>(gfx::Result::NotReady), 0);
}

TEST(GfxCppUtilTest, ResultToStringWithIsSuccess)
{
    // Demonstrate combined usage with success checking
    gfx::Result result = gfx::Result::Success;
    if (gfx::isSuccess(result)) {
        std::string msg = gfx::utils::resultToString(result);
        EXPECT_STREQ(msg.c_str(), "Result::Success");
    }
}

} // namespace
