#include <backend/webgpu/core/render/RenderPass.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPURenderPassTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::webgpu::core::Device>(adapter, deviceInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "WebGPU not available: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::webgpu::core::Device> device;
};

TEST_F(WebGPURenderPassTest, CreateRenderPass_WithColorAttachment)
{
    gfx::backend::webgpu::core::RenderPassCreateInfo createInfo{};

    gfx::backend::webgpu::core::RenderPassColorAttachment colorAttachment{};
    colorAttachment.format = WGPUTextureFormat_RGBA8Unorm;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    createInfo.colorAttachments.push_back(colorAttachment);

    auto renderPass = std::make_unique<gfx::backend::webgpu::core::RenderPass>(device.get(), createInfo);

    EXPECT_EQ(renderPass->getDevice(), device.get());
    EXPECT_EQ(renderPass->getCreateInfo().colorAttachments.size(), 1);
}

TEST_F(WebGPURenderPassTest, GetDevice_ReturnsCorrectDevice)
{
    gfx::backend::webgpu::core::RenderPassCreateInfo createInfo{};

    gfx::backend::webgpu::core::RenderPassColorAttachment colorAttachment{};
    colorAttachment.format = WGPUTextureFormat_BGRA8Unorm;
    colorAttachment.loadOp = WGPULoadOp_Load;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    createInfo.colorAttachments.push_back(colorAttachment);

    auto renderPass = std::make_unique<gfx::backend::webgpu::core::RenderPass>(device.get(), createInfo);

    EXPECT_EQ(renderPass->getDevice(), device.get());
}

TEST_F(WebGPURenderPassTest, CreateRenderPass_WithDepthStencil)
{
    gfx::backend::webgpu::core::RenderPassCreateInfo createInfo{};

    gfx::backend::webgpu::core::RenderPassColorAttachment colorAttachment{};
    colorAttachment.format = WGPUTextureFormat_RGBA8Unorm;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    createInfo.colorAttachments.push_back(colorAttachment);

    gfx::backend::webgpu::core::RenderPassDepthStencilAttachment depthStencil{};
    depthStencil.format = WGPUTextureFormat_Depth24PlusStencil8;
    depthStencil.depthLoadOp = WGPULoadOp_Clear;
    depthStencil.depthStoreOp = WGPUStoreOp_Store;
    depthStencil.stencilLoadOp = WGPULoadOp_Clear;
    depthStencil.stencilStoreOp = WGPUStoreOp_Store;
    createInfo.depthStencilAttachment = depthStencil;

    auto renderPass = std::make_unique<gfx::backend::webgpu::core::RenderPass>(device.get(), createInfo);

    EXPECT_EQ(renderPass->getDevice(), device.get());
    EXPECT_TRUE(renderPass->getCreateInfo().depthStencilAttachment.has_value());
}

TEST_F(WebGPURenderPassTest, Destructor_CleansUpResources)
{
    {
        gfx::backend::webgpu::core::RenderPassCreateInfo createInfo{};

        gfx::backend::webgpu::core::RenderPassColorAttachment colorAttachment{};
        colorAttachment.format = WGPUTextureFormat_RGBA8Unorm;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        createInfo.colorAttachments.push_back(colorAttachment);

        auto renderPass = std::make_unique<gfx::backend::webgpu::core::RenderPass>(device.get(), createInfo);
        EXPECT_EQ(renderPass->getDevice(), device.get());
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
