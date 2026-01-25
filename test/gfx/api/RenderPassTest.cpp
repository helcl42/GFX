#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxRenderPassTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        GfxInstanceDescriptor instDesc = {};
        instDesc.backend = backend;
        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.label = "Test Device";

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxAdapterDestroy(adapter);
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
        if (adapter) {
            gfxAdapterDestroy(adapter);
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
// RenderPass Tests
// ===========================================================================

// Test: Create RenderPass with NULL device
TEST_P(GfxRenderPassTest, CreateRenderPassWithNullDevice)
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
    GfxResult result = gfxDeviceCreateRenderPass(nullptr, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create RenderPass with NULL descriptor
TEST_P(GfxRenderPassTest, CreateRenderPassWithNullDescriptor)
{
    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, nullptr, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create RenderPass with NULL output
TEST_P(GfxRenderPassTest, CreateRenderPassWithNullOutput)
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

    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create basic RenderPass with single color attachment
TEST_P(GfxRenderPassTest, CreateBasicRenderPass)
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
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPass with multiple color attachments
TEST_P(GfxRenderPassTest, CreateRenderPassWithMultipleColorAttachments)
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
    renderPassDesc.label = "Multiple Color Attachments";
    renderPassDesc.colorAttachments = colorAttachments;
    renderPassDesc.colorAttachmentCount = 2;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPass with depth attachment
TEST_P(GfxRenderPassTest, CreateRenderPassWithDepthAttachment)
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
    renderPassDesc.label = "Depth Render Pass";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPass with depth-stencil attachment
TEST_P(GfxRenderPassTest, CreateRenderPassWithDepthStencilAttachment)
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

    GfxRenderPassDepthStencilAttachmentTarget depthStencilTarget = {};
    depthStencilTarget.format = GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    depthStencilTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    depthStencilTarget.depthOps.loadOp = GFX_LOAD_OP_CLEAR;
    depthStencilTarget.depthOps.storeOp = GFX_STORE_OP_STORE;
    depthStencilTarget.stencilOps.loadOp = GFX_LOAD_OP_CLEAR;
    depthStencilTarget.stencilOps.storeOp = GFX_STORE_OP_STORE;
    depthStencilTarget.finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

    GfxRenderPassDepthStencilAttachment depthStencilAttachment = {};
    depthStencilAttachment.target = depthStencilTarget;
    depthStencilAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.label = "Depth Stencil Render Pass";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPass with different load operations
TEST_P(GfxRenderPassTest, CreateRenderPassWithDifferentLoadOps)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_LOAD; // Load existing content
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.label = "Load Op Test";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPass with DONT_CARE operations
TEST_P(GfxRenderPassTest, CreateRenderPassWithDontCareOps)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_DONT_CARE;
    colorTarget.ops.storeOp = GFX_STORE_OP_DONT_CARE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.label = "Dont Care Ops Test";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPass with different texture formats
TEST_P(GfxRenderPassTest, CreateRenderPassWithDifferentFormats)
{
    GfxTextureFormat formats[] = {
        GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
        GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
        GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT,
        GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT
    };

    for (GfxTextureFormat format : formats) {
        GfxRenderPassColorAttachmentTarget colorTarget = {};
        colorTarget.format = format;
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
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(renderPass, nullptr);

        if (renderPass) {
            gfxRenderPassDestroy(renderPass);
        }
    }
}

// Test: Create RenderPass with multisampling
TEST_P(GfxRenderPassTest, CreateRenderPassWithMultisampling)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_4;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.label = "Multisampled Render Pass";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    gfxRenderPassDestroy(renderPass);
}

// Test: Destroy NULL RenderPass
TEST_P(GfxRenderPassTest, DestroyNullRenderPass)
{
    GfxResult result = gfxRenderPassDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxRenderPassTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
