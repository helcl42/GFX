#include "CommonTest.h"

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxFramebufferTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        GfxInstanceDescriptor instDesc = {};
        instDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
        instDesc.pNext = nullptr;
        instDesc.backend = backend;
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
        deviceDesc.pNext = nullptr;
        deviceDesc.label = "Test Device";

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
    }

    void TearDown() override
    {
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend = GFX_BACKEND_VULKAN;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

// ===========================================================================
// Framebuffer Tests
// ===========================================================================

// Test: Create Framebuffer with NULL device
TEST_P(GfxFramebufferTest, CreateFramebufferWithNullDevice)
{
    // Create render pass
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create texture
    GfxTextureDescriptor texDesc = {};
    texDesc.type = GFX_TEXTURE_TYPE_2D;
    texDesc.size = { 256, 256, 1 };
    texDesc.arrayLayerCount = 1;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    texDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture texture = nullptr;
    result = gfxDeviceCreateTexture(device, &texDesc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create texture view
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView textureView = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &textureView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create framebuffer
    GfxFramebufferAttachment fbAttachment = {};
    fbAttachment.view = textureView;
    fbAttachment.resolveTarget = nullptr;

    GfxFramebufferDescriptor fbDesc = {};
    fbDesc.renderPass = renderPass;
    fbDesc.colorAttachments = &fbAttachment;
    fbDesc.colorAttachmentCount = 1;
    fbDesc.depthStencilAttachment = { nullptr, nullptr };
    fbDesc.extent.width = 256;
    fbDesc.extent.height = 256;

    GfxFramebuffer framebuffer = nullptr;
    result = gfxDeviceCreateFramebuffer(nullptr, &fbDesc, &framebuffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxTextureViewDestroy(textureView);
    gfxTextureDestroy(texture);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create Framebuffer with NULL descriptor
TEST_P(GfxFramebufferTest, CreateFramebufferWithNullDescriptor)
{
    GfxFramebuffer framebuffer = nullptr;
    GfxResult result = gfxDeviceCreateFramebuffer(device, nullptr, &framebuffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create Framebuffer with NULL output
TEST_P(GfxFramebufferTest, CreateFramebufferWithNullOutput)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureDescriptor texDesc = {};
    texDesc.type = GFX_TEXTURE_TYPE_2D;
    texDesc.size = { 256, 256, 1 };
    texDesc.arrayLayerCount = 1;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    texDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture texture = nullptr;
    result = gfxDeviceCreateTexture(device, &texDesc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView textureView = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &textureView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxFramebufferAttachment fbAttachment = {};
    fbAttachment.view = textureView;
    fbAttachment.resolveTarget = nullptr;

    GfxFramebufferDescriptor fbDesc = {};
    fbDesc.renderPass = renderPass;
    fbDesc.colorAttachments = &fbAttachment;
    fbDesc.colorAttachmentCount = 1;
    fbDesc.depthStencilAttachment = { nullptr, nullptr };
    fbDesc.extent.width = 256;
    fbDesc.extent.height = 256;

    result = gfxDeviceCreateFramebuffer(device, &fbDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxTextureViewDestroy(textureView);
    gfxTextureDestroy(texture);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create basic Framebuffer with single color attachment
TEST_P(GfxFramebufferTest, CreateBasicFramebuffer)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.label = "Basic Render Pass";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(renderPass, nullptr);

    GfxTextureDescriptor texDesc = {};
    texDesc.type = GFX_TEXTURE_TYPE_2D;
    texDesc.size = { 256, 256, 1 };
    texDesc.arrayLayerCount = 1;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    texDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture texture = nullptr;
    result = gfxDeviceCreateTexture(device, &texDesc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView textureView = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &textureView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(textureView, nullptr);

    GfxFramebufferAttachment fbAttachment = {};
    fbAttachment.view = textureView;
    fbAttachment.resolveTarget = nullptr;

    GfxFramebufferDescriptor fbDesc = {};
    fbDesc.label = "Basic Framebuffer";
    fbDesc.renderPass = renderPass;
    fbDesc.colorAttachments = &fbAttachment;
    fbDesc.colorAttachmentCount = 1;
    fbDesc.depthStencilAttachment = { nullptr, nullptr };
    fbDesc.extent.width = 256;
    fbDesc.extent.height = 256;

    GfxFramebuffer framebuffer = nullptr;
    result = gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(framebuffer, nullptr);

    gfxFramebufferDestroy(framebuffer);
    gfxTextureViewDestroy(textureView);
    gfxTextureDestroy(texture);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create Framebuffer with multiple color attachments
TEST_P(GfxFramebufferTest, CreateFramebufferWithMultipleColorAttachments)
{
    GfxRenderPassColorAttachmentTarget colorTargets[2] = {};

    colorTargets[0].format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTargets[0].sampleCount = GFX_SAMPLE_COUNT_1;
    colorTargets[0].ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTargets[0].ops.storeOp = GFX_STORE_OP_STORE;
    colorTargets[0].finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    colorTargets[1].format = GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT;
    colorTargets[1].sampleCount = GFX_SAMPLE_COUNT_1;
    colorTargets[1].ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTargets[1].ops.storeOp = GFX_STORE_OP_STORE;
    colorTargets[1].finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachments[2] = {};
    colorAttachments[0].target = colorTargets[0];
    colorAttachments[0].resolveTarget = nullptr;
    colorAttachments[1].target = colorTargets[1];
    colorAttachments[1].resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = colorAttachments;
    renderPassDesc.colorAttachmentCount = 2;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create textures
    GfxTextureDescriptor texDesc1 = {};
    texDesc1.type = GFX_TEXTURE_TYPE_2D;
    texDesc1.size = { 512, 512, 1 };
    texDesc1.arrayLayerCount = 1;
    texDesc1.mipLevelCount = 1;
    texDesc1.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc1.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    texDesc1.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture texture1 = nullptr;
    result = gfxDeviceCreateTexture(device, &texDesc1, &texture1);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureDescriptor texDesc2 = {};
    texDesc2.type = GFX_TEXTURE_TYPE_2D;
    texDesc2.size = { 512, 512, 1 };
    texDesc2.arrayLayerCount = 1;
    texDesc2.mipLevelCount = 1;
    texDesc2.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc2.format = GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT;
    texDesc2.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture texture2 = nullptr;
    result = gfxDeviceCreateTexture(device, &texDesc2, &texture2);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create texture views
    GfxTextureViewDescriptor viewDesc1 = {};
    viewDesc1.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc1.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc1.baseMipLevel = 0;
    viewDesc1.mipLevelCount = 1;
    viewDesc1.baseArrayLayer = 0;
    viewDesc1.arrayLayerCount = 1;

    GfxTextureView textureView1 = nullptr;
    result = gfxTextureCreateView(texture1, &viewDesc1, &textureView1);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc2 = {};
    viewDesc2.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc2.format = GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT;
    viewDesc2.baseMipLevel = 0;
    viewDesc2.mipLevelCount = 1;
    viewDesc2.baseArrayLayer = 0;
    viewDesc2.arrayLayerCount = 1;

    GfxTextureView textureView2 = nullptr;
    result = gfxTextureCreateView(texture2, &viewDesc2, &textureView2);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create framebuffer
    GfxFramebufferAttachment fbAttachments[2] = {};
    fbAttachments[0].view = textureView1;
    fbAttachments[0].resolveTarget = nullptr;
    fbAttachments[1].view = textureView2;
    fbAttachments[1].resolveTarget = nullptr;

    GfxFramebufferDescriptor fbDesc = {};
    fbDesc.label = "Multiple Attachments Framebuffer";
    fbDesc.renderPass = renderPass;
    fbDesc.colorAttachments = fbAttachments;
    fbDesc.colorAttachmentCount = 2;
    fbDesc.depthStencilAttachment = { nullptr, nullptr };
    fbDesc.extent.width = 512;
    fbDesc.extent.height = 512;

    GfxFramebuffer framebuffer = nullptr;
    result = gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(framebuffer, nullptr);

    gfxFramebufferDestroy(framebuffer);
    gfxTextureViewDestroy(textureView2);
    gfxTextureViewDestroy(textureView1);
    gfxTextureDestroy(texture2);
    gfxTextureDestroy(texture1);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create Framebuffer with depth attachment
TEST_P(GfxFramebufferTest, CreateFramebufferWithDepthAttachment)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDepthStencilAttachmentTarget depthTarget = {};
    depthTarget.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    depthTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    depthTarget.depthOps.loadOp = GFX_LOAD_OP_CLEAR;
    depthTarget.depthOps.storeOp = GFX_STORE_OP_STORE;
    depthTarget.stencilOps.loadOp = GFX_LOAD_OP_DONT_CARE;
    depthTarget.stencilOps.storeOp = GFX_STORE_OP_DONT_CARE;
    depthTarget.finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

    GfxRenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.target = depthTarget;
    depthAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create color texture
    GfxTextureDescriptor colorTexDesc = {};
    colorTexDesc.type = GFX_TEXTURE_TYPE_2D;
    colorTexDesc.size = { 1024, 768, 1 };
    colorTexDesc.arrayLayerCount = 1;
    colorTexDesc.mipLevelCount = 1;
    colorTexDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTexDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTexDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture colorTexture = nullptr;
    result = gfxDeviceCreateTexture(device, &colorTexDesc, &colorTexture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor colorViewDesc = {};
    colorViewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    colorViewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorViewDesc.baseMipLevel = 0;
    colorViewDesc.mipLevelCount = 1;
    colorViewDesc.baseArrayLayer = 0;
    colorViewDesc.arrayLayerCount = 1;

    GfxTextureView colorView = nullptr;
    result = gfxTextureCreateView(colorTexture, &colorViewDesc, &colorView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create depth texture
    GfxTextureDescriptor depthTexDesc = {};
    depthTexDesc.type = GFX_TEXTURE_TYPE_2D;
    depthTexDesc.size = { 1024, 768, 1 };
    depthTexDesc.arrayLayerCount = 1;
    depthTexDesc.mipLevelCount = 1;
    depthTexDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    depthTexDesc.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    depthTexDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture depthTexture = nullptr;
    result = gfxDeviceCreateTexture(device, &depthTexDesc, &depthTexture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor depthViewDesc = {};
    depthViewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    depthViewDesc.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    depthViewDesc.baseMipLevel = 0;
    depthViewDesc.mipLevelCount = 1;
    depthViewDesc.baseArrayLayer = 0;
    depthViewDesc.arrayLayerCount = 1;

    GfxTextureView depthView = nullptr;
    result = gfxTextureCreateView(depthTexture, &depthViewDesc, &depthView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create framebuffer
    GfxFramebufferAttachment colorFbAttachment = {};
    colorFbAttachment.view = colorView;
    colorFbAttachment.resolveTarget = nullptr;

    GfxFramebufferDescriptor fbDesc = {};
    fbDesc.label = "Depth Framebuffer";
    fbDesc.renderPass = renderPass;
    fbDesc.colorAttachments = &colorFbAttachment;
    fbDesc.colorAttachmentCount = 1;
    fbDesc.depthStencilAttachment = { depthView, nullptr };
    fbDesc.extent.width = 1024;
    fbDesc.extent.height = 768;

    GfxFramebuffer framebuffer = nullptr;
    result = gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(framebuffer, nullptr);

    gfxFramebufferDestroy(framebuffer);
    gfxTextureViewDestroy(depthView);
    gfxTextureViewDestroy(colorView);
    gfxTextureDestroy(depthTexture);
    gfxTextureDestroy(colorTexture);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create Framebuffer with different sizes
TEST_P(GfxFramebufferTest, CreateFramebufferWithDifferentSizes)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    struct {
        uint32_t width;
        uint32_t height;
    } sizes[] = {
        { 128, 128 },
        { 256, 256 },
        { 512, 512 },
        { 1920, 1080 },
        { 3840, 2160 }
    };

    for (auto size : sizes) {
        GfxTextureDescriptor texDesc = {};
        texDesc.type = GFX_TEXTURE_TYPE_2D;
        texDesc.size = { size.width, size.height, 1 };
        texDesc.arrayLayerCount = 1;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = GFX_SAMPLE_COUNT_1;
        texDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        texDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

        GfxTexture texture = nullptr;
        result = gfxDeviceCreateTexture(device, &texDesc, &texture);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);

        GfxTextureViewDescriptor viewDesc = {};
        viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
        viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;

        GfxTextureView textureView = nullptr;
        result = gfxTextureCreateView(texture, &viewDesc, &textureView);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);

        GfxFramebufferAttachment fbAttachment = {};
        fbAttachment.view = textureView;
        fbAttachment.resolveTarget = nullptr;

        GfxFramebufferDescriptor fbDesc = {};
        fbDesc.renderPass = renderPass;
        fbDesc.colorAttachments = &fbAttachment;
        fbDesc.colorAttachmentCount = 1;
        fbDesc.depthStencilAttachment = { nullptr, nullptr };
        fbDesc.extent = { size.width, size.height };

        GfxFramebuffer framebuffer = nullptr;
        result = gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffer);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(framebuffer, nullptr);

        if (framebuffer) {
            gfxFramebufferDestroy(framebuffer);
        }
        gfxTextureViewDestroy(textureView);
        gfxTextureDestroy(texture);
    }

    gfxRenderPassDestroy(renderPass);
}

// Test: Destroy NULL Framebuffer
TEST_P(GfxFramebufferTest, DestroyNullFramebuffer)
{
    GfxResult result = gfxFramebufferDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxFramebufferTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
