#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

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

// ===========================================================================
// Parameterized Tests - Access flags differ between Vulkan and WebGPU
// ===========================================================================

class GfxCppUtilAccessFlagsTest : public testing::TestWithParam<gfx::Backend> {
protected:
    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;

    void SetUp() override
    {
        backend = GetParam();
        
        // Create instance to establish backend context
        try {
            gfx::InstanceDescriptor instanceDesc{};
            instanceDesc.backend = backend;
            instance = gfx::createInstance(instanceDesc);
        } catch (...) {
            GTEST_SKIP() << "Backend not available";
        }
    }

    void TearDown() override
    {
        instance.reset();
    }
};

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutUndefined)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::Undefined);
    EXPECT_EQ(flags, gfx::AccessFlags::None);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutGeneral)
{
    // Note: gfxGetAccessFlagsForLayout always queries Vulkan backend globally,
    // so it returns Vulkan flags regardless of which backend we're testing with
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::General);
    EXPECT_EQ(flags, gfx::AccessFlags::MemoryRead | gfx::AccessFlags::MemoryWrite);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutColorAttachment)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::ColorAttachment);
    EXPECT_EQ(flags, gfx::AccessFlags::ColorAttachmentRead | gfx::AccessFlags::ColorAttachmentWrite);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutDepthStencil)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::DepthStencilAttachment);
    EXPECT_EQ(flags, gfx::AccessFlags::DepthStencilAttachmentRead | gfx::AccessFlags::DepthStencilAttachmentWrite);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutDepthStencilReadOnly)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::DepthStencilReadOnly);
    EXPECT_EQ(flags, gfx::AccessFlags::DepthStencilAttachmentRead);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutShaderReadOnly)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::ShaderReadOnly);
    EXPECT_EQ(flags, gfx::AccessFlags::ShaderRead);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutTransferSrc)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::TransferSrc);
    EXPECT_EQ(flags, gfx::AccessFlags::TransferRead);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutTransferDst)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::TransferDst);
    EXPECT_EQ(flags, gfx::AccessFlags::TransferWrite);
}

TEST_P(GfxCppUtilAccessFlagsTest, GetAccessFlagsForLayoutPresent)
{
    gfx::AccessFlags flags = gfx::utils::getAccessFlagsForLayout(gfx::TextureLayout::PresentSrc);
    EXPECT_EQ(flags, gfx::AccessFlags::MemoryRead);
}

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppUtilAccessFlagsTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

// ===========================================================================
// Non-parameterized Tests - Backend-independent utility functions
// ===========================================================================

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
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromWayland(surface, display);
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
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromWin32(hwnd, hinstance);
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
    void* layer = (void*)0x1234;
    gfx::PlatformWindowHandle handle = gfx::PlatformWindowHandle::fromMetal(layer);
    EXPECT_EQ(handle.windowingSystem, gfx::WindowingSystem::Metal);
    EXPECT_EQ(handle.handle.metal.layer, layer);
}

} // namespace
