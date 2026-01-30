#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// RenderPass Test Suite
// ===========================================================================

namespace {

class GfxCppRenderPassTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();
        try {
            gfx::InstanceDescriptor instanceDesc{
                .backend = backend
            };
            instance = gfx::createInstance(instanceDesc);

            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0
            };
            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };
            device = adapter->createDevice(deviceDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up GFX device for backend "
                         << (backend == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU")
                         << ": " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// ===========================================================================
// Test Cases
// ===========================================================================

// Test: Create basic RenderPass with single color attachment
TEST_P(GfxCppRenderPassTest, CreateBasicRenderPass)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Basic Render Pass",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with multiple color attachments
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithMultipleColorAttachments)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Multiple Color Attachments",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } },
            gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R16G16B16A16Float, .sampleCount = gfx::SampleCount::Count1, .loadOp = gfx::LoadOp::Clear, .storeOp = gfx::StoreOp::Store, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with depth attachment
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithDepthAttachment)
{
    gfx::RenderPassDepthStencilAttachment depthAttachment = {
        .target = {
            .format = gfx::TextureFormat::Depth32Float,
            .sampleCount = gfx::SampleCount::Count1,
            .depthLoadOp = gfx::LoadOp::Clear,
            .depthStoreOp = gfx::StoreOp::Store,
            .stencilLoadOp = gfx::LoadOp::DontCare,
            .stencilStoreOp = gfx::StoreOp::DontCare,
            .finalLayout = gfx::TextureLayout::DepthStencilAttachment }
    };

    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Depth Render Pass",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } },
        .depthStencilAttachment = &depthAttachment
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with depth-stencil attachment
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithDepthStencilAttachment)
{
    gfx::RenderPassDepthStencilAttachment depthStencilAttachment = {
        .target = {
            .format = gfx::TextureFormat::Depth24PlusStencil8,
            .sampleCount = gfx::SampleCount::Count1,
            .depthLoadOp = gfx::LoadOp::Clear,
            .depthStoreOp = gfx::StoreOp::Store,
            .stencilLoadOp = gfx::LoadOp::Clear,
            .stencilStoreOp = gfx::StoreOp::Store,
            .finalLayout = gfx::TextureLayout::DepthStencilAttachment }
    };

    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Depth Stencil Render Pass",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } },
        .depthStencilAttachment = &depthStencilAttachment
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with different load operations
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithDifferentLoadOps)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Load Op Test",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Load, // Load existing content
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with DONT_CARE operations
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithDontCareOps)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Dont Care Ops Test",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::DontCare,
                    .storeOp = gfx::StoreOp::DontCare,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with different texture formats
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithDifferentFormats)
{
    gfx::TextureFormat formats[] = {
        gfx::TextureFormat::R8G8B8A8Unorm,
        gfx::TextureFormat::B8G8R8A8Unorm,
        gfx::TextureFormat::R16G16B16A16Float,
        gfx::TextureFormat::R32G32B32A32Float
    };

    for (auto format : formats) {
        gfx::RenderPassCreateDescriptor renderPassDesc{
            .colorAttachments = {
                gfx::RenderPassColorAttachment{
                    .target = {
                        .format = format,
                        .sampleCount = gfx::SampleCount::Count1,
                        .loadOp = gfx::LoadOp::Clear,
                        .storeOp = gfx::StoreOp::Store,
                        .finalLayout = gfx::TextureLayout::ColorAttachment } } }
        };

        auto renderPass = device->createRenderPass(renderPassDesc);
        EXPECT_NE(renderPass, nullptr);
    }
}

// Test: Create RenderPass with multisampling
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithMultisampling)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Multisampled Render Pass",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count4,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// Test: Create RenderPass with empty label
TEST_P(GfxCppRenderPassTest, CreateRenderPassWithEmptyLabel)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppRenderPassTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
