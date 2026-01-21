#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxDeviceTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        GfxInstanceDescriptor instDesc = {};
        instDesc.backend = backend;
        instDesc.enableValidation = false;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.adapterIndex = UINT32_MAX;
        adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to get adapter";
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

    GfxBackend backend;
    GfxInstance instance = NULL;
    GfxAdapter adapter = NULL;
    GfxDevice device = NULL;
};

TEST_P(GfxDeviceTest, CreateDestroyDevice)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(device, nullptr);
}

TEST_P(GfxDeviceTest, CreateDeviceInvalidArguments)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    // NULL output pointer
    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL adapter
    result = gfxAdapterCreateDevice(NULL, &desc, &device);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxAdapterCreateDevice(adapter, NULL, &device);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxDeviceTest, GetDefaultQueue)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(device, nullptr);

    GfxQueue queue = NULL;
    result = gfxDeviceGetQueue(device, &queue);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(queue, nullptr);
}

TEST_P(GfxDeviceTest, GetQueueByIndex)
{
    // Get queue families first
    uint32_t queueFamilyCount = 0;
    GfxResult result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, NULL);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    if (queueFamilyCount == 0) {
        GTEST_SKIP() << "No queue families available";
    }

    GfxQueueFamilyProperties* queueFamilies = new GfxQueueFamilyProperties[queueFamilyCount];
    result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, queueFamilies);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create device
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Try to get queue from first family
    GfxQueue queue = NULL;
    result = gfxDeviceGetQueueByIndex(device, 0, 0, &queue);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(queue, nullptr);

    delete[] queueFamilies;
}

TEST_P(GfxDeviceTest, GetQueueInvalidArguments)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // NULL device
    GfxQueue queue = NULL;
    result = gfxDeviceGetQueue(NULL, &queue);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxDeviceGetQueue(device, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL device for GetQueueByIndex
    result = gfxDeviceGetQueueByIndex(NULL, 0, 0, &queue);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer for GetQueueByIndex
    result = gfxDeviceGetQueueByIndex(device, 0, 0, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxDeviceTest, GetQueueInvalidIndex)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Try to get queue with invalid family index
    GfxQueue queue = NULL;
    result = gfxDeviceGetQueueByIndex(device, 9999, 0, &queue);

    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxDeviceTest, WaitIdle)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    result = gfxDeviceWaitIdle(device);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxDeviceTest, GetLimits)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxDeviceLimits limits = {};
    result = gfxDeviceGetLimits(device, &limits);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_GT(limits.maxBufferSize, 0u);
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
}

TEST_P(GfxDeviceTest, CreateBuffer)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 1024;
    bufferDesc.usage = static_cast<GfxBufferUsage>(GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST);

    GfxBuffer buffer = NULL;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(buffer, nullptr);

    if (buffer) {
        gfxBufferDestroy(buffer);
    }
}

TEST_P(GfxDeviceTest, CreateTexture)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureDescriptor texDesc = {};
    texDesc.type = GFX_TEXTURE_TYPE_2D;
    texDesc.size = { 256, 256, 1 };
    texDesc.arrayLayerCount = 1;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    texDesc.usage = static_cast<GfxTextureUsage>(GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_DST);

    GfxTexture texture = NULL;
    result = gfxDeviceCreateTexture(device, &texDesc, &texture);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(texture, nullptr);

    if (texture) {
        gfxTextureDestroy(texture);
    }
}

TEST_P(GfxDeviceTest, CreateSampler)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerDesc.addressModeV = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerDesc.addressModeW = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerDesc.magFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.minFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.mipmapFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1000.0f;
    samplerDesc.compare = GFX_COMPARE_FUNCTION_UNDEFINED;
    samplerDesc.maxAnisotropy = 1;

    GfxSampler sampler = NULL;
    result = gfxDeviceCreateSampler(device, &samplerDesc, &sampler);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(sampler, nullptr);

    if (sampler) {
        gfxSamplerDestroy(sampler);
    }
}

TEST_P(GfxDeviceTest, CreateCommandEncoder)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxCommandEncoderDescriptor encDesc = {};

    GfxCommandEncoder encoder = NULL;
    result = gfxDeviceCreateCommandEncoder(device, &encDesc, &encoder);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(encoder, nullptr);

    if (encoder) {
        gfxCommandEncoderDestroy(encoder);
    }
}

TEST_P(GfxDeviceTest, CreateFence)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.signaled = false;

    GfxFence fence = NULL;
    result = gfxDeviceCreateFence(device, &fenceDesc, &fence);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(fence, nullptr);

    if (fence) {
        gfxFenceDestroy(fence);
    }
}

TEST_P(GfxDeviceTest, CreateSemaphore)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxSemaphoreDescriptor semDesc = {};
    semDesc.type = GFX_SEMAPHORE_TYPE_BINARY;
    semDesc.initialValue = 0;

    GfxSemaphore semaphore = NULL;
    result = gfxDeviceCreateSemaphore(device, &semDesc, &semaphore);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(semaphore, nullptr);

    if (semaphore) {
        gfxSemaphoreDestroy(semaphore);
    }
}

TEST_P(GfxDeviceTest, MultipleDevices)
{
    // WebGPU backend doesn't support multiple devices from the same adapter
    if (backend == GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WebGPU doesn't support multiple devices from same adapter";
    }

    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxDevice device1 = NULL;
    GfxDevice device2 = NULL;

    GfxResult result1 = gfxAdapterCreateDevice(adapter, &desc, &device1);
    GfxResult result2 = gfxAdapterCreateDevice(adapter, &desc, &device2);

    EXPECT_EQ(result1, GFX_RESULT_SUCCESS);
    EXPECT_EQ(result2, GFX_RESULT_SUCCESS);
    EXPECT_NE(device1, nullptr);
    EXPECT_NE(device2, nullptr);
    EXPECT_NE(device1, device2);

    if (device1)
        gfxDeviceDestroy(device1);
    if (device2)
        gfxDeviceDestroy(device2);
}

TEST_P(GfxDeviceTest, CreateShader)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Simple SPIR-V shader code (vertex shader that does nothing)
    const uint32_t spirvCode[] = {
        0x07230203, 0x00010000, 0x00080001, 0x00000006,
        0x00000000, 0x00020011, 0x00000001, 0x0003000e,
        0x00000000, 0x00000001
    };

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
    shaderDesc.code = spirvCode;
    shaderDesc.codeSize = sizeof(spirvCode);
    shaderDesc.entryPoint = "main";

    GfxShader shader = NULL;
    result = gfxDeviceCreateShader(device, &shaderDesc, &shader);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(shader, nullptr);

    if (shader) {
        gfxShaderDestroy(shader);
    }
}

TEST_P(GfxDeviceTest, CreateBindGroupLayout)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_VERTEX;
    entry.type = GFX_BINDING_TYPE_BUFFER;
    entry.buffer.hasDynamicOffset = false;
    entry.buffer.minBindingSize = 64;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &entry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = NULL;
    result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxDeviceTest, CreateBindGroup)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create bind group layout
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 64;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = NULL;
    result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create buffer for bind group
    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;

    GfxBuffer buffer = NULL;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create bind group
    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry.resource.buffer.buffer = buffer;
    entry.resource.buffer.offset = 0;
    entry.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = NULL;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    if (bindGroup) {
        gfxBindGroupDestroy(bindGroup);
    }
    if (buffer) {
        gfxBufferDestroy(buffer);
    }
    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxDeviceTest, CreateRenderPass)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = NULL;

    GfxRenderPassDescriptor rpDesc = {};
    rpDesc.colorAttachments = &colorAttachment;
    rpDesc.colorAttachmentCount = 1;
    rpDesc.depthStencilAttachment = NULL;

    GfxRenderPass renderPass = NULL;
    result = gfxDeviceCreateRenderPass(device, &rpDesc, &renderPass);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(renderPass, nullptr);

    if (renderPass) {
        gfxRenderPassDestroy(renderPass);
    }
}

TEST_P(GfxDeviceTest, CreateFramebuffer)
{
    GfxDeviceDescriptor desc = {};
    desc.queuePriority = 1.0f;

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create render pass
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = NULL;

    GfxRenderPassDescriptor rpDesc = {};
    rpDesc.colorAttachments = &colorAttachment;
    rpDesc.colorAttachmentCount = 1;
    rpDesc.depthStencilAttachment = NULL;

    GfxRenderPass renderPass = NULL;
    result = gfxDeviceCreateRenderPass(device, &rpDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create texture for framebuffer
    GfxTextureDescriptor texDesc = {};
    texDesc.type = GFX_TEXTURE_TYPE_2D;
    texDesc.size = { 256, 256, 1 };
    texDesc.arrayLayerCount = 1;
    texDesc.mipLevelCount = 1;
    texDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    texDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    texDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture texture = NULL;
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

    GfxTextureView textureView = NULL;
    result = gfxTextureCreateView(texture, &viewDesc, &textureView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create framebuffer
    GfxFramebufferAttachment fbAttachment = {};
    fbAttachment.view = textureView;
    fbAttachment.resolveTarget = NULL;

    GfxFramebufferDescriptor fbDesc = {};
    fbDesc.renderPass = renderPass;
    fbDesc.colorAttachments = &fbAttachment;
    fbDesc.colorAttachmentCount = 1;
    fbDesc.depthStencilAttachment = { NULL, NULL };
    fbDesc.width = 256;
    fbDesc.height = 256;

    GfxFramebuffer framebuffer = NULL;
    result = gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffer);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(framebuffer, nullptr);

    if (framebuffer) {
        gfxFramebufferDestroy(framebuffer);
    }
    if (textureView) {
        gfxTextureViewDestroy(textureView);
    }
    if (texture) {
        gfxTextureDestroy(texture);
    }
    if (renderPass) {
        gfxRenderPassDestroy(renderPass);
    }
}

// Instantiate tests for both backends
INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxDeviceTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

// ===========================================================================
// Non-Parameterized Tests - Backend-independent functionality
// ===========================================================================

TEST(GfxDeviceTestNonParam, DestroyNullDevice)
{
    GfxResult result = gfxDeviceDestroy(NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}
