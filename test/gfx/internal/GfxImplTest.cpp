#include <gfx/gfx.h>

// Include internal headers for testing
#include "../../../gfx/src/backend/Factory.h"
#include "../../../gfx/src/backend/IBackend.h"
#include "../../../gfx/src/backend/Manager.h"

#include <gtest/gtest.h>

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace gfx::backend::test {

// Mock Backend Implementation
class MockBackend : public IBackend {
public:
    // Instance functions
    MOCK_METHOD(GfxResult, createInstance, (const GfxInstanceDescriptor*, GfxInstance*), (const, override));
    MOCK_METHOD(GfxResult, instanceDestroy, (GfxInstance), (const, override));
    MOCK_METHOD(GfxResult, instanceRequestAdapter, (GfxInstance, const GfxAdapterDescriptor*, GfxAdapter*), (const, override));
    MOCK_METHOD(GfxResult, instanceEnumerateAdapters, (GfxInstance, uint32_t*, GfxAdapter*), (const, override));
    MOCK_METHOD(GfxResult, enumerateInstanceExtensions, (uint32_t*, const char**), (const, override));

    // Adapter functions
    MOCK_METHOD(GfxResult, adapterCreateDevice, (GfxAdapter, const GfxDeviceDescriptor*, GfxDevice*), (const, override));
    MOCK_METHOD(GfxResult, adapterGetInfo, (GfxAdapter, GfxAdapterInfo*), (const, override));
    MOCK_METHOD(GfxResult, adapterGetLimits, (GfxAdapter, GfxDeviceLimits*), (const, override));
    MOCK_METHOD(GfxResult, adapterEnumerateQueueFamilies, (GfxAdapter, uint32_t*, GfxQueueFamilyProperties*), (const, override));
    MOCK_METHOD(GfxResult, adapterGetQueueFamilySurfaceSupport, (GfxAdapter, uint32_t, GfxSurface, bool*), (const, override));
    MOCK_METHOD(GfxResult, adapterEnumerateExtensions, (GfxAdapter, uint32_t*, const char**), (const, override));

    // Device functions
    MOCK_METHOD(GfxResult, deviceDestroy, (GfxDevice), (const, override));
    MOCK_METHOD(GfxResult, deviceGetQueue, (GfxDevice, GfxQueue*), (const, override));
    MOCK_METHOD(GfxResult, deviceGetQueueByIndex, (GfxDevice, uint32_t, uint32_t, GfxQueue*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateSurface, (GfxDevice, const GfxSurfaceDescriptor*, GfxSurface*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateSwapchain, (GfxDevice, const GfxSwapchainDescriptor*, GfxSwapchain*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateBuffer, (GfxDevice, const GfxBufferDescriptor*, GfxBuffer*), (const, override));
    MOCK_METHOD(GfxResult, deviceImportBuffer, (GfxDevice, const GfxBufferImportDescriptor*, GfxBuffer*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateTexture, (GfxDevice, const GfxTextureDescriptor*, GfxTexture*), (const, override));
    MOCK_METHOD(GfxResult, deviceImportTexture, (GfxDevice, const GfxTextureImportDescriptor*, GfxTexture*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateSampler, (GfxDevice, const GfxSamplerDescriptor*, GfxSampler*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateShader, (GfxDevice, const GfxShaderDescriptor*, GfxShader*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateBindGroupLayout, (GfxDevice, const GfxBindGroupLayoutDescriptor*, GfxBindGroupLayout*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateBindGroup, (GfxDevice, const GfxBindGroupDescriptor*, GfxBindGroup*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateRenderPipeline, (GfxDevice, const GfxRenderPipelineDescriptor*, GfxRenderPipeline*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateComputePipeline, (GfxDevice, const GfxComputePipelineDescriptor*, GfxComputePipeline*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateCommandEncoder, (GfxDevice, const GfxCommandEncoderDescriptor*, GfxCommandEncoder*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateRenderPass, (GfxDevice, const GfxRenderPassDescriptor*, GfxRenderPass*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateFramebuffer, (GfxDevice, const GfxFramebufferDescriptor*, GfxFramebuffer*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateFence, (GfxDevice, const GfxFenceDescriptor*, GfxFence*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateSemaphore, (GfxDevice, const GfxSemaphoreDescriptor*, GfxSemaphore*), (const, override));
    MOCK_METHOD(GfxResult, deviceCreateQuerySet, (GfxDevice, const GfxQuerySetDescriptor*, GfxQuerySet*), (const, override));
    MOCK_METHOD(GfxResult, deviceWaitIdle, (GfxDevice), (const, override));
    MOCK_METHOD(GfxResult, deviceGetLimits, (GfxDevice, GfxDeviceLimits*), (const, override));

    // Surface functions
    MOCK_METHOD(GfxResult, surfaceDestroy, (GfxSurface), (const, override));
    MOCK_METHOD(GfxResult, surfaceEnumerateSupportedFormats, (GfxSurface, uint32_t*, GfxTextureFormat*), (const, override));
    MOCK_METHOD(GfxResult, surfaceEnumerateSupportedPresentModes, (GfxSurface, uint32_t*, GfxPresentMode*), (const, override));

    // Swapchain functions
    MOCK_METHOD(GfxResult, swapchainDestroy, (GfxSwapchain), (const, override));
    MOCK_METHOD(GfxResult, swapchainGetInfo, (GfxSwapchain, GfxSwapchainInfo*), (const, override));
    MOCK_METHOD(GfxResult, swapchainAcquireNextImage, (GfxSwapchain, uint64_t, GfxSemaphore, GfxFence, uint32_t*), (const, override));
    MOCK_METHOD(GfxResult, swapchainGetTextureView, (GfxSwapchain, uint32_t, GfxTextureView*), (const, override));
    MOCK_METHOD(GfxResult, swapchainGetCurrentTextureView, (GfxSwapchain, GfxTextureView*), (const, override));
    MOCK_METHOD(GfxResult, swapchainPresent, (GfxSwapchain, const GfxPresentInfo*), (const, override));

    // Buffer functions
    MOCK_METHOD(GfxResult, bufferDestroy, (GfxBuffer), (const, override));
    MOCK_METHOD(GfxResult, bufferGetInfo, (GfxBuffer, GfxBufferInfo*), (const, override));
    MOCK_METHOD(GfxResult, bufferGetNativeHandle, (GfxBuffer, void**), (const, override));
    MOCK_METHOD(GfxResult, bufferMap, (GfxBuffer, uint64_t, uint64_t, void**), (const, override));
    MOCK_METHOD(GfxResult, bufferUnmap, (GfxBuffer), (const, override));

    // Texture functions
    MOCK_METHOD(GfxResult, textureDestroy, (GfxTexture), (const, override));
    MOCK_METHOD(GfxResult, textureGetInfo, (GfxTexture, GfxTextureInfo*), (const, override));
    MOCK_METHOD(GfxResult, textureGetNativeHandle, (GfxTexture, void**), (const, override));
    MOCK_METHOD(GfxResult, textureGetLayout, (GfxTexture, GfxTextureLayout*), (const, override));
    MOCK_METHOD(GfxResult, textureCreateView, (GfxTexture, const GfxTextureViewDescriptor*, GfxTextureView*), (const, override));

    // TextureView functions
    MOCK_METHOD(GfxResult, textureViewDestroy, (GfxTextureView), (const, override));

    // Sampler functions
    MOCK_METHOD(GfxResult, samplerDestroy, (GfxSampler), (const, override));

    // Shader functions
    MOCK_METHOD(GfxResult, shaderDestroy, (GfxShader), (const, override));

    // BindGroupLayout functions
    MOCK_METHOD(GfxResult, bindGroupLayoutDestroy, (GfxBindGroupLayout), (const, override));

    // BindGroup functions
    MOCK_METHOD(GfxResult, bindGroupDestroy, (GfxBindGroup), (const, override));

    // RenderPipeline functions
    MOCK_METHOD(GfxResult, renderPipelineDestroy, (GfxRenderPipeline), (const, override));

    // ComputePipeline functions
    MOCK_METHOD(GfxResult, computePipelineDestroy, (GfxComputePipeline), (const, override));

    // RenderPass functions
    MOCK_METHOD(GfxResult, renderPassDestroy, (GfxRenderPass), (const, override));

    // Framebuffer functions
    MOCK_METHOD(GfxResult, framebufferDestroy, (GfxFramebuffer), (const, override));

    // CommandEncoder functions
    MOCK_METHOD(GfxResult, commandEncoderDestroy, (GfxCommandEncoder), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderBeginRenderPass, (GfxCommandEncoder, const GfxRenderPassBeginDescriptor*, GfxRenderPassEncoder*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderBeginComputePass, (GfxCommandEncoder, const GfxComputePassBeginDescriptor*, GfxComputePassEncoder*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderCopyBufferToBuffer, (GfxCommandEncoder, const GfxCopyBufferToBufferDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderCopyBufferToTexture, (GfxCommandEncoder, const GfxCopyBufferToTextureDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderCopyTextureToBuffer, (GfxCommandEncoder, const GfxCopyTextureToBufferDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderCopyTextureToTexture, (GfxCommandEncoder, const GfxCopyTextureToTextureDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderBlitTextureToTexture, (GfxCommandEncoder, const GfxBlitTextureToTextureDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderPipelineBarrier, (GfxCommandEncoder, const GfxPipelineBarrierDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderGenerateMipmaps, (GfxCommandEncoder, GfxTexture), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderGenerateMipmapsRange, (GfxCommandEncoder, GfxTexture, uint32_t, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderWriteTimestamp, (GfxCommandEncoder, GfxQuerySet, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderResolveQuerySet, (GfxCommandEncoder, GfxQuerySet, uint32_t, uint32_t, GfxBuffer, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderEnd, (GfxCommandEncoder), (const, override));
    MOCK_METHOD(GfxResult, commandEncoderBegin, (GfxCommandEncoder), (const, override));

    // RenderPassEncoder functions
    MOCK_METHOD(GfxResult, renderPassEncoderSetPipeline, (GfxRenderPassEncoder, GfxRenderPipeline), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderSetBindGroup, (GfxRenderPassEncoder, uint32_t, GfxBindGroup, const uint32_t*, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderSetVertexBuffer, (GfxRenderPassEncoder, uint32_t, GfxBuffer, uint64_t, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderSetIndexBuffer, (GfxRenderPassEncoder, GfxBuffer, GfxIndexFormat, uint64_t, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderSetViewport, (GfxRenderPassEncoder, const GfxViewport*), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderSetScissorRect, (GfxRenderPassEncoder, const GfxScissorRect*), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderDraw, (GfxRenderPassEncoder, uint32_t, uint32_t, uint32_t, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderDrawIndexed, (GfxRenderPassEncoder, uint32_t, uint32_t, uint32_t, int32_t, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderDrawIndirect, (GfxRenderPassEncoder, GfxBuffer, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderDrawIndexedIndirect, (GfxRenderPassEncoder, GfxBuffer, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderEnd, (GfxRenderPassEncoder), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderBeginOcclusionQuery, (GfxRenderPassEncoder, GfxQuerySet, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, renderPassEncoderEndOcclusionQuery, (GfxRenderPassEncoder), (const, override));

    // ComputePassEncoder functions
    MOCK_METHOD(GfxResult, computePassEncoderSetPipeline, (GfxComputePassEncoder, GfxComputePipeline), (const, override));
    MOCK_METHOD(GfxResult, computePassEncoderSetBindGroup, (GfxComputePassEncoder, uint32_t, GfxBindGroup, const uint32_t*, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, computePassEncoderDispatch, (GfxComputePassEncoder, uint32_t, uint32_t, uint32_t), (const, override));
    MOCK_METHOD(GfxResult, computePassEncoderDispatchIndirect, (GfxComputePassEncoder, GfxBuffer, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, computePassEncoderEnd, (GfxComputePassEncoder), (const, override));

    // Queue functions
    MOCK_METHOD(GfxResult, queueSubmit, (GfxQueue, const GfxSubmitDescriptor*), (const, override));
    MOCK_METHOD(GfxResult, queueWriteBuffer, (GfxQueue, GfxBuffer, uint64_t, const void*, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, queueWriteTexture, (GfxQueue, GfxTexture, const GfxOrigin3D*, uint32_t, const void*, uint64_t, const GfxExtent3D*, GfxTextureLayout), (const, override));
    MOCK_METHOD(GfxResult, queueWaitIdle, (GfxQueue), (const, override));

    // Fence functions
    MOCK_METHOD(GfxResult, fenceDestroy, (GfxFence), (const, override));
    MOCK_METHOD(GfxResult, fenceGetStatus, (GfxFence, bool*), (const, override));
    MOCK_METHOD(GfxResult, fenceWait, (GfxFence, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, fenceReset, (GfxFence), (const, override));

    // Semaphore functions
    MOCK_METHOD(GfxResult, semaphoreDestroy, (GfxSemaphore), (const, override));
    MOCK_METHOD(GfxResult, semaphoreGetType, (GfxSemaphore, GfxSemaphoreType*), (const, override));
    MOCK_METHOD(GfxResult, semaphoreSignal, (GfxSemaphore, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, semaphoreWait, (GfxSemaphore, uint64_t, uint64_t), (const, override));
    MOCK_METHOD(GfxResult, semaphoreGetValue, (GfxSemaphore, uint64_t*), (const, override));

    // QuerySet functions
    MOCK_METHOD(GfxResult, querySetDestroy, (GfxQuerySet), (const, override));

    // Helper functions
    MOCK_METHOD(GfxAccessFlags, getAccessFlagsForLayout, (GfxTextureLayout), (const, override));
};

// Test Fixture
class GfxImplTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Unload all backends before each test
        gfxUnloadAllBackends();
    }

    void TearDown() override
    {
        // Clean up after each test
        gfxUnloadAllBackends();
    }
};

// ============================================================================
// Version Tests
// ============================================================================

TEST_F(GfxImplTest, GetVersion_ValidPointers_ReturnsSuccess)
{
    uint32_t major, minor, patch;
    ASSERT_EQ(gfxGetVersion(&major, &minor, &patch), GFX_RESULT_SUCCESS);
    EXPECT_EQ(major, GFX_VERSION_MAJOR);
    EXPECT_EQ(minor, GFX_VERSION_MINOR);
    EXPECT_EQ(patch, GFX_VERSION_PATCH);
}

TEST_F(GfxImplTest, GetVersion_NullMajor_ReturnsError)
{
    uint32_t minor, patch;
    ASSERT_EQ(gfxGetVersion(nullptr, &minor, &patch), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, GetVersion_NullMinor_ReturnsError)
{
    uint32_t major, patch;
    ASSERT_EQ(gfxGetVersion(&major, nullptr, &patch), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, GetVersion_NullPatch_ReturnsError)
{
    uint32_t major, minor;
    ASSERT_EQ(gfxGetVersion(&major, &minor, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Backend Loading Tests
// ============================================================================

TEST_F(GfxImplTest, LoadBackend_InvalidBackend_ReturnsError)
{
    ASSERT_EQ(gfxLoadBackend(static_cast<GfxBackend>(999)), GFX_RESULT_ERROR_INVALID_ARGUMENT);
    ASSERT_EQ(gfxLoadBackend(static_cast<GfxBackend>(-1)), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, UnloadBackend_InvalidBackend_ReturnsError)
{
    ASSERT_EQ(gfxUnloadBackend(static_cast<GfxBackend>(999)), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, LoadBackend_MultipleLoads_Vulkan_Succeeds)
{
#ifdef GFX_ENABLE_VULKAN
    // First load
    GfxResult result = gfxLoadBackend(GFX_BACKEND_VULKAN);
    ASSERT_TRUE(result == GFX_RESULT_SUCCESS || result == GFX_RESULT_ERROR_BACKEND_NOT_LOADED);

    if (result == GFX_RESULT_SUCCESS) {
        // Second load (should succeed as it's already loaded)
        ASSERT_EQ(gfxLoadBackend(GFX_BACKEND_VULKAN), GFX_RESULT_SUCCESS);
    }
#else
    GTEST_SKIP() << "Vulkan backend not enabled";
#endif
}

TEST_F(GfxImplTest, LoadBackend_MultipleLoads_WebGPU_Succeeds)
{
#ifdef GFX_ENABLE_WEBGPU
    // First load
    GfxResult result = gfxLoadBackend(GFX_BACKEND_WEBGPU);
    ASSERT_TRUE(result == GFX_RESULT_SUCCESS || result == GFX_RESULT_ERROR_BACKEND_NOT_LOADED);

    if (result == GFX_RESULT_SUCCESS) {
        // Second load (should succeed as it's already loaded)
        ASSERT_EQ(gfxLoadBackend(GFX_BACKEND_WEBGPU), GFX_RESULT_SUCCESS);
    }
#else
    GTEST_SKIP() << "WebGPU backend not enabled";
#endif
}

TEST_F(GfxImplTest, UnloadBackend_NotLoaded_Vulkan_Succeeds)
{
    // Unloading a backend that's not loaded should succeed (idempotent)
#ifdef GFX_ENABLE_VULKAN
    ASSERT_EQ(gfxUnloadBackend(GFX_BACKEND_VULKAN), GFX_RESULT_SUCCESS);
#else
    GTEST_SKIP() << "Vulkan backend not enabled";
#endif
}

TEST_F(GfxImplTest, UnloadBackend_NotLoaded_WebGPU_Succeeds)
{
    // Unloading a backend that's not loaded should succeed (idempotent)
#ifdef GFX_ENABLE_WEBGPU
    ASSERT_EQ(gfxUnloadBackend(GFX_BACKEND_WEBGPU), GFX_RESULT_SUCCESS);
#else
    GTEST_SKIP() << "WebGPU backend not enabled";
#endif
}

// ============================================================================
// Instance Creation Tests
// ============================================================================

TEST_F(GfxImplTest, CreateInstance_NullDescriptor_ReturnsError)
{
    GfxInstance instance;
    ASSERT_EQ(gfxCreateInstance(nullptr, &instance), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CreateInstance_NullOutInstance_ReturnsError)
{
    GfxInstanceDescriptor desc = {};
    desc.backend = GFX_BACKEND_AUTO;
    ASSERT_EQ(gfxCreateInstance(&desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CreateInstance_BackendNotLoaded_ReturnsError)
{
    gfxUnloadAllBackends();

    GfxInstanceDescriptor desc = {};
    desc.backend = GFX_BACKEND_AUTO;
    GfxInstance instance;
    ASSERT_EQ(gfxCreateInstance(&desc, &instance), GFX_RESULT_ERROR_BACKEND_NOT_LOADED);
}

#ifdef GFX_ENABLE_VULKAN
TEST_F(GfxImplTest, CreateInstance_VulkanBackend_Succeeds)
{
    GfxResult loadResult = gfxLoadBackend(GFX_BACKEND_VULKAN);
    if (loadResult != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Vulkan backend could not be loaded";
    }

    GfxInstanceDescriptor desc = {};
    desc.backend = GFX_BACKEND_VULKAN;
    desc.applicationName = "GfxImplTest";
    desc.applicationVersion = 1;
    desc.enabledExtensions = nullptr;
    desc.enabledExtensionCount = 0;

    GfxInstance instance;
    GfxResult result = gfxCreateInstance(&desc, &instance);

    if (result == GFX_RESULT_SUCCESS) {
        ASSERT_NE(instance, nullptr);
        ASSERT_EQ(gfxInstanceDestroy(instance), GFX_RESULT_SUCCESS);
    }
}
#endif

#ifdef GFX_ENABLE_WEBGPU
TEST_F(GfxImplTest, CreateInstance_WebGPUBackend_Succeeds)
{
    GfxResult loadResult = gfxLoadBackend(GFX_BACKEND_WEBGPU);
    if (loadResult != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "WebGPU backend could not be loaded";
    }

    GfxInstanceDescriptor desc = {};
    desc.backend = GFX_BACKEND_WEBGPU;
    desc.applicationName = "GfxImplTest";
    desc.applicationVersion = 1;
    desc.enabledExtensions = nullptr;
    desc.enabledExtensionCount = 0;

    GfxInstance instance;
    GfxResult result = gfxCreateInstance(&desc, &instance);

    if (result == GFX_RESULT_SUCCESS) {
        ASSERT_NE(instance, nullptr);
        ASSERT_EQ(gfxInstanceDestroy(instance), GFX_RESULT_SUCCESS);
    }
}
#endif

// ============================================================================
// Instance Destroy Tests
// ============================================================================

TEST_F(GfxImplTest, InstanceDestroy_NullInstance_ReturnsError)
{
    ASSERT_EQ(gfxInstanceDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, InstanceDestroy_InvalidInstance_ReturnsError)
{
    // Create a bogus instance handle
    GfxInstance bogus = reinterpret_cast<GfxInstance>(0xDEADBEEF);
    ASSERT_EQ(gfxInstanceDestroy(bogus), GFX_RESULT_ERROR_NOT_FOUND);
}

// ============================================================================
// Adapter Request Tests
// ============================================================================

TEST_F(GfxImplTest, RequestAdapter_NullInstance_ReturnsError)
{
    GfxAdapterDescriptor desc = {};
    GfxAdapter adapter;
    ASSERT_EQ(gfxInstanceRequestAdapter(nullptr, &desc, &adapter), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RequestAdapter_NullDescriptor_ReturnsError)
{
    GfxInstance instance = reinterpret_cast<GfxInstance>(0x1);
    GfxAdapter adapter;
    ASSERT_EQ(gfxInstanceRequestAdapter(instance, nullptr, &adapter), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RequestAdapter_NullOutAdapter_ReturnsError)
{
    GfxInstance instance = reinterpret_cast<GfxInstance>(0x1);
    GfxAdapterDescriptor desc = {};
    ASSERT_EQ(gfxInstanceRequestAdapter(instance, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Enumerate Adapters Tests
// ============================================================================

TEST_F(GfxImplTest, EnumerateAdapters_NullInstance_ReturnsError)
{
    uint32_t count;
    ASSERT_EQ(gfxInstanceEnumerateAdapters(nullptr, &count, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, EnumerateAdapters_NullCount_ReturnsError)
{
    GfxInstance instance = reinterpret_cast<GfxInstance>(0x1);
    ASSERT_EQ(gfxInstanceEnumerateAdapters(instance, nullptr, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Load All Backends Tests
// ============================================================================

TEST_F(GfxImplTest, LoadAllBackends_LoadsAvailableBackends)
{
    GfxResult result = gfxLoadAllBackends();

#if defined(GFX_ENABLE_VULKAN) || defined(GFX_ENABLE_WEBGPU)
    // At least one backend should be available
    ASSERT_TRUE(result == GFX_RESULT_SUCCESS || result == GFX_RESULT_ERROR_BACKEND_NOT_LOADED);
#else
    ASSERT_EQ(result, GFX_RESULT_ERROR_BACKEND_NOT_LOADED);
#endif
}

TEST_F(GfxImplTest, UnloadAllBackends_UnloadsAllBackends)
{
    // Load all backends first
    gfxLoadAllBackends();

    // Unload all
    ASSERT_EQ(gfxUnloadAllBackends(), GFX_RESULT_SUCCESS);

    // Try to create instance - should fail since all backends are unloaded
    GfxInstanceDescriptor desc = {};
    desc.backend = GFX_BACKEND_AUTO;
    GfxInstance instance;
    ASSERT_EQ(gfxCreateInstance(&desc, &instance), GFX_RESULT_ERROR_BACKEND_NOT_LOADED);
}

// ============================================================================
// Adapter Info Tests
// ============================================================================

TEST_F(GfxImplTest, AdapterGetInfo_NullAdapter_ReturnsError)
{
    GfxAdapterInfo info;
    ASSERT_EQ(gfxAdapterGetInfo(nullptr, &info), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterGetInfo_NullOutInfo_ReturnsError)
{
    GfxAdapter adapter = reinterpret_cast<GfxAdapter>(0x1);
    ASSERT_EQ(gfxAdapterGetInfo(adapter, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterGetLimits_NullAdapter_ReturnsError)
{
    GfxDeviceLimits limits;
    ASSERT_EQ(gfxAdapterGetLimits(nullptr, &limits), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterGetLimits_NullOutLimits_ReturnsError)
{
    GfxAdapter adapter = reinterpret_cast<GfxAdapter>(0x1);
    ASSERT_EQ(gfxAdapterGetLimits(adapter, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Device Creation Tests
// ============================================================================

TEST_F(GfxImplTest, AdapterCreateDevice_NullAdapter_ReturnsError)
{
    GfxDeviceDescriptor desc = {};
    GfxDevice device;
    ASSERT_EQ(gfxAdapterCreateDevice(nullptr, &desc, &device), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterCreateDevice_NullDescriptor_ReturnsError)
{
    GfxAdapter adapter = reinterpret_cast<GfxAdapter>(0x1);
    GfxDevice device;
    ASSERT_EQ(gfxAdapterCreateDevice(adapter, nullptr, &device), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterCreateDevice_NullOutDevice_ReturnsError)
{
    GfxAdapter adapter = reinterpret_cast<GfxAdapter>(0x1);
    GfxDeviceDescriptor desc = {};
    ASSERT_EQ(gfxAdapterCreateDevice(adapter, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceDestroy_NullDevice_ReturnsError)
{
    ASSERT_EQ(gfxDeviceDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Buffer Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateBuffer_NullDevice_ReturnsError)
{
    GfxBufferDescriptor desc = {};
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;
    GfxBuffer buffer;
    ASSERT_EQ(gfxDeviceCreateBuffer(nullptr, &desc, &buffer), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBuffer_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxBuffer buffer;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, nullptr, &buffer), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBuffer_NullOutBuffer_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxBufferDescriptor desc = {};
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferDestroy_NullBuffer_ReturnsError)
{
    ASSERT_EQ(gfxBufferDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferMap_NullBuffer_ReturnsError)
{
    void* mapped;
    ASSERT_EQ(gfxBufferMap(nullptr, 0, 0, &mapped), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferMap_NullOutPointer_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxBufferMap(buffer, 0, 0, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferUnmap_NullBuffer_ReturnsError)
{
    ASSERT_EQ(gfxBufferUnmap(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Texture Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateTexture_NullDevice_ReturnsError)
{
    GfxTextureDescriptor desc = {};
    GfxTexture texture;
    ASSERT_EQ(gfxDeviceCreateTexture(nullptr, &desc, &texture), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateTexture_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxTexture texture;
    ASSERT_EQ(gfxDeviceCreateTexture(device, nullptr, &texture), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateTexture_NullOutTexture_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxTextureDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateTexture(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureDestroy_NullTexture_ReturnsError)
{
    ASSERT_EQ(gfxTextureDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureCreateView_NullTexture_ReturnsError)
{
    GfxTextureViewDescriptor desc = {};
    GfxTextureView view;
    ASSERT_EQ(gfxTextureCreateView(nullptr, &desc, &view), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureCreateView_NullOutView_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    GfxTextureViewDescriptor desc = {};
    ASSERT_EQ(gfxTextureCreateView(texture, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Shader Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateShader_NullDevice_ReturnsError)
{
    GfxShaderDescriptor desc = {};
    GfxShader shader;
    ASSERT_EQ(gfxDeviceCreateShader(nullptr, &desc, &shader), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateShader_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxShader shader;
    ASSERT_EQ(gfxDeviceCreateShader(device, nullptr, &shader), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateShader_NullOutShader_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxShaderDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateShader(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, ShaderDestroy_NullShader_ReturnsError)
{
    ASSERT_EQ(gfxShaderDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Pipeline Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateRenderPipeline_NullDevice_ReturnsError)
{
    GfxRenderPipelineDescriptor desc = {};
    GfxRenderPipeline pipeline;
    ASSERT_EQ(gfxDeviceCreateRenderPipeline(nullptr, &desc, &pipeline), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateRenderPipeline_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxRenderPipeline pipeline;
    ASSERT_EQ(gfxDeviceCreateRenderPipeline(device, nullptr, &pipeline), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateRenderPipeline_NullOutPipeline_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxRenderPipelineDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateRenderPipeline(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateComputePipeline_NullDevice_ReturnsError)
{
    GfxComputePipelineDescriptor desc = {};
    GfxComputePipeline pipeline;
    ASSERT_EQ(gfxDeviceCreateComputePipeline(nullptr, &desc, &pipeline), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateComputePipeline_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxComputePipeline pipeline;
    ASSERT_EQ(gfxDeviceCreateComputePipeline(device, nullptr, &pipeline), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateComputePipeline_NullOutPipeline_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxComputePipelineDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateComputePipeline(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Command Encoder Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateCommandEncoder_NullDevice_ReturnsError)
{
    GfxCommandEncoderDescriptor desc = {};
    GfxCommandEncoder encoder;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(nullptr, &desc, &encoder), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateCommandEncoder_NullOutEncoder_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxCommandEncoderDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderDestroy_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxCommandEncoderDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderBegin_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxCommandEncoderBegin(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderEnd_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxCommandEncoderEnd(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Queue Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceGetQueue_NullDevice_ReturnsError)
{
    GfxQueue queue;
    ASSERT_EQ(gfxDeviceGetQueue(nullptr, &queue), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceGetQueue_NullOutQueue_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    ASSERT_EQ(gfxDeviceGetQueue(device, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, QueueSubmit_NullQueue_ReturnsError)
{
    GfxSubmitDescriptor desc = {};
    ASSERT_EQ(gfxQueueSubmit(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, QueueWaitIdle_NullQueue_ReturnsError)
{
    ASSERT_EQ(gfxQueueWaitIdle(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Swapchain Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateSwapchain_NullDevice_ReturnsError)
{
    GfxSurface surface = reinterpret_cast<GfxSurface>(0x1);
    GfxSwapchainDescriptor desc = {};
    desc.surface = surface;
    GfxSwapchain swapchain;
    ASSERT_EQ(gfxDeviceCreateSwapchain(nullptr, &desc, &swapchain), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSwapchain_NullSurface_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSwapchainDescriptor desc = {};
    desc.surface = nullptr;
    GfxSwapchain swapchain;
    ASSERT_EQ(gfxDeviceCreateSwapchain(device, &desc, &swapchain), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSwapchain_NullOutSwapchain_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSurface surface = reinterpret_cast<GfxSurface>(0x1);
    GfxSwapchainDescriptor desc = {};
    desc.surface = surface;
    ASSERT_EQ(gfxDeviceCreateSwapchain(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SwapchainDestroy_NullSwapchain_ReturnsError)
{
    ASSERT_EQ(gfxSwapchainDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Fence Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateFence_NullDevice_ReturnsError)
{
    GfxFenceDescriptor desc = {};
    GfxFence fence;
    ASSERT_EQ(gfxDeviceCreateFence(nullptr, &desc, &fence), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateFence_NullOutFence_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxFenceDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateFence(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, FenceDestroy_NullFence_ReturnsError)
{
    ASSERT_EQ(gfxFenceDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, FenceWait_NullFence_ReturnsError)
{
    ASSERT_EQ(gfxFenceWait(nullptr, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Semaphore Tests
// ============================================================================

TEST_F(GfxImplTest, DeviceCreateSemaphore_NullDevice_ReturnsError)
{
    GfxSemaphoreDescriptor desc = {};
    GfxSemaphore semaphore;
    ASSERT_EQ(gfxDeviceCreateSemaphore(nullptr, &desc, &semaphore), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSemaphore_NullOutSemaphore_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSemaphoreDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateSemaphore(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SemaphoreDestroy_NullSemaphore_ReturnsError)
{
    ASSERT_EQ(gfxSemaphoreDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ============================================================================
// Additional Missing Function Tests
// ============================================================================

// Instance Extensions
TEST_F(GfxImplTest, EnumerateInstanceExtensions_NullCount_ReturnsError)
{
    ASSERT_EQ(gfxEnumerateInstanceExtensions(GFX_BACKEND_VULKAN, nullptr, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Adapter Extensions & Queue Families
TEST_F(GfxImplTest, AdapterEnumerateQueueFamilies_NullAdapter_ReturnsError)
{
    uint32_t count;
    ASSERT_EQ(gfxAdapterEnumerateQueueFamilies(nullptr, &count, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterEnumerateQueueFamilies_NullCount_ReturnsError)
{
    GfxAdapter adapter = reinterpret_cast<GfxAdapter>(0x1);
    ASSERT_EQ(gfxAdapterEnumerateQueueFamilies(adapter, nullptr, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterGetQueueFamilySurfaceSupport_NullAdapter_ReturnsError)
{
    bool supported;
    GfxSurface surface = reinterpret_cast<GfxSurface>(0x1);
    ASSERT_EQ(gfxAdapterGetQueueFamilySurfaceSupport(nullptr, 0, surface, &supported), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, AdapterEnumerateExtensions_NullAdapter_ReturnsError)
{
    uint32_t count;
    ASSERT_EQ(gfxAdapterEnumerateExtensions(nullptr, &count, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Device Queue by Index
TEST_F(GfxImplTest, DeviceGetQueueByIndex_NullDevice_ReturnsError)
{
    GfxQueue queue;
    ASSERT_EQ(gfxDeviceGetQueueByIndex(nullptr, 0, 0, &queue), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceGetQueueByIndex_NullOutQueue_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    ASSERT_EQ(gfxDeviceGetQueueByIndex(device, 0, 0, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Surface Creation & Info
TEST_F(GfxImplTest, DeviceCreateSurface_NullDevice_ReturnsError)
{
    GfxSurfaceDescriptor desc = {};
    GfxSurface surface;
    ASSERT_EQ(gfxDeviceCreateSurface(nullptr, &desc, &surface), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSurface_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSurface surface;
    ASSERT_EQ(gfxDeviceCreateSurface(device, nullptr, &surface), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSurface_NullOutSurface_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSurfaceDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateSurface(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SurfaceDestroy_NullSurface_ReturnsError)
{
    ASSERT_EQ(gfxSurfaceDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SurfaceEnumerateSupportedFormats_NullSurface_ReturnsError)
{
    uint32_t count;
    ASSERT_EQ(gfxSurfaceEnumerateSupportedFormats(nullptr, &count, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SurfaceEnumerateSupportedPresentModes_NullSurface_ReturnsError)
{
    uint32_t count;
    ASSERT_EQ(gfxSurfaceEnumerateSupportedPresentModes(nullptr, &count, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Buffer Import & Info
TEST_F(GfxImplTest, DeviceImportBuffer_NullDevice_ReturnsError)
{
    GfxBufferImportDescriptor desc = {};
    GfxBuffer buffer;
    ASSERT_EQ(gfxDeviceImportBuffer(nullptr, &desc, &buffer), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferGetInfo_NullBuffer_ReturnsError)
{
    GfxBufferInfo info;
    ASSERT_EQ(gfxBufferGetInfo(nullptr, &info), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferGetInfo_NullOutInfo_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxBufferGetInfo(buffer, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferGetNativeHandle_NullBuffer_ReturnsError)
{
    void* handle;
    ASSERT_EQ(gfxBufferGetNativeHandle(nullptr, &handle), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BufferGetNativeHandle_NullOutHandle_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxBufferGetNativeHandle(buffer, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Texture Import & Info
TEST_F(GfxImplTest, DeviceImportTexture_NullDevice_ReturnsError)
{
    GfxTextureImportDescriptor desc = {};
    GfxTexture texture;
    ASSERT_EQ(gfxDeviceImportTexture(nullptr, &desc, &texture), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureGetInfo_NullTexture_ReturnsError)
{
    GfxTextureInfo info;
    ASSERT_EQ(gfxTextureGetInfo(nullptr, &info), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureGetInfo_NullOutInfo_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    ASSERT_EQ(gfxTextureGetInfo(texture, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureGetNativeHandle_NullTexture_ReturnsError)
{
    void* handle;
    ASSERT_EQ(gfxTextureGetNativeHandle(nullptr, &handle), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureGetNativeHandle_NullOutHandle_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    ASSERT_EQ(gfxTextureGetNativeHandle(texture, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureGetLayout_NullTexture_ReturnsError)
{
    GfxTextureLayout layout;
    ASSERT_EQ(gfxTextureGetLayout(nullptr, &layout), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureGetLayout_NullOutLayout_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    ASSERT_EQ(gfxTextureGetLayout(texture, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, TextureViewDestroy_NullTextureView_ReturnsError)
{
    ASSERT_EQ(gfxTextureViewDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Sampler & BindGroupLayout
TEST_F(GfxImplTest, DeviceCreateSampler_NullDevice_ReturnsError)
{
    GfxSamplerDescriptor desc = {};
    GfxSampler sampler;
    ASSERT_EQ(gfxDeviceCreateSampler(nullptr, &desc, &sampler), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSampler_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSampler sampler;
    ASSERT_EQ(gfxDeviceCreateSampler(device, nullptr, &sampler), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateSampler_NullOutSampler_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxSamplerDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateSampler(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SamplerDestroy_NullSampler_ReturnsError)
{
    ASSERT_EQ(gfxSamplerDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBindGroupLayout_NullDevice_ReturnsError)
{
    GfxBindGroupLayoutDescriptor desc = {};
    GfxBindGroupLayout layout;
    ASSERT_EQ(gfxDeviceCreateBindGroupLayout(nullptr, &desc, &layout), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBindGroupLayout_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxBindGroupLayout layout;
    ASSERT_EQ(gfxDeviceCreateBindGroupLayout(device, nullptr, &layout), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBindGroupLayout_NullOutLayout_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxBindGroupLayoutDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateBindGroupLayout(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BindGroupLayoutDestroy_NullBindGroupLayout_ReturnsError)
{
    ASSERT_EQ(gfxBindGroupLayoutDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// BindGroup
TEST_F(GfxImplTest, DeviceCreateBindGroup_NullDevice_ReturnsError)
{
    GfxBindGroupDescriptor desc = {};
    GfxBindGroup bindGroup;
    ASSERT_EQ(gfxDeviceCreateBindGroup(nullptr, &desc, &bindGroup), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBindGroup_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxBindGroup bindGroup;
    ASSERT_EQ(gfxDeviceCreateBindGroup(device, nullptr, &bindGroup), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateBindGroup_NullOutBindGroup_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxBindGroupDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateBindGroup(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, BindGroupDestroy_NullBindGroup_ReturnsError)
{
    ASSERT_EQ(gfxBindGroupDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// RenderPass & Framebuffer
TEST_F(GfxImplTest, DeviceCreateRenderPass_NullDevice_ReturnsError)
{
    GfxRenderPassDescriptor desc = {};
    GfxRenderPass renderPass;
    ASSERT_EQ(gfxDeviceCreateRenderPass(nullptr, &desc, &renderPass), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateRenderPass_NullOutRenderPass_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxRenderPassDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateRenderPass(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassDestroy_NullRenderPass_ReturnsError)
{
    ASSERT_EQ(gfxRenderPassDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateFramebuffer_NullDevice_ReturnsError)
{
    GfxFramebufferDescriptor desc = {};
    GfxFramebuffer framebuffer;
    ASSERT_EQ(gfxDeviceCreateFramebuffer(nullptr, &desc, &framebuffer), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateFramebuffer_NullOutFramebuffer_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxFramebufferDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateFramebuffer(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, FramebufferDestroy_NullFramebuffer_ReturnsError)
{
    ASSERT_EQ(gfxFramebufferDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// QuerySet
TEST_F(GfxImplTest, DeviceCreateQuerySet_NullDevice_ReturnsError)
{
    GfxQuerySetDescriptor desc = {};
    GfxQuerySet querySet;
    ASSERT_EQ(gfxDeviceCreateQuerySet(nullptr, &desc, &querySet), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateQuerySet_NullDescriptor_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxQuerySet querySet;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, nullptr, &querySet), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceCreateQuerySet_NullOutQuerySet_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    GfxQuerySetDescriptor desc = {};
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &desc, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, QuerySetDestroy_NullQuerySet_ReturnsError)
{
    ASSERT_EQ(gfxQuerySetDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceWaitIdle_NullDevice_ReturnsError)
{
    ASSERT_EQ(gfxDeviceWaitIdle(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceGetLimits_NullDevice_ReturnsError)
{
    GfxDeviceLimits limits;
    ASSERT_EQ(gfxDeviceGetLimits(nullptr, &limits), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, DeviceGetLimits_NullOutLimits_ReturnsError)
{
    GfxDevice device = reinterpret_cast<GfxDevice>(0x1);
    ASSERT_EQ(gfxDeviceGetLimits(device, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Pipeline Destroy
TEST_F(GfxImplTest, RenderPipelineDestroy_NullPipeline_ReturnsError)
{
    ASSERT_EQ(gfxRenderPipelineDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, ComputePipelineDestroy_NullPipeline_ReturnsError)
{
    ASSERT_EQ(gfxComputePipelineDestroy(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Swapchain Operations
TEST_F(GfxImplTest, SwapchainGetInfo_NullSwapchain_ReturnsError)
{
    GfxSwapchainInfo info;
    ASSERT_EQ(gfxSwapchainGetInfo(nullptr, &info), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SwapchainGetInfo_NullOutInfo_ReturnsError)
{
    GfxSwapchain swapchain = reinterpret_cast<GfxSwapchain>(0x1);
    ASSERT_EQ(gfxSwapchainGetInfo(swapchain, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SwapchainAcquireNextImage_NullSwapchain_ReturnsError)
{
    uint32_t imageIndex;
    ASSERT_EQ(gfxSwapchainAcquireNextImage(nullptr, 0, nullptr, nullptr, &imageIndex), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SwapchainGetTextureView_NullSwapchain_ReturnsError)
{
    GfxTextureView view;
    ASSERT_EQ(gfxSwapchainGetTextureView(nullptr, 0, &view), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SwapchainGetCurrentTextureView_NullSwapchain_ReturnsError)
{
    GfxTextureView view;
    ASSERT_EQ(gfxSwapchainGetCurrentTextureView(nullptr, &view), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SwapchainPresent_NullSwapchain_ReturnsError)
{
    ASSERT_EQ(gfxSwapchainPresent(nullptr, nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Queue Write Operations
TEST_F(GfxImplTest, QueueWriteBuffer_NullQueue_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    uint8_t data = 0;
    ASSERT_EQ(gfxQueueWriteBuffer(nullptr, buffer, 0, &data, 1), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, QueueWriteTexture_NullQueue_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    uint8_t data = 0;
    GfxOrigin3D origin = {};
    GfxExtent3D extent = {};
    ASSERT_EQ(gfxQueueWriteTexture(nullptr, texture, &origin, 0, &data, 1, &extent, GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Command Encoder Copy Operations
TEST_F(GfxImplTest, CommandEncoderCopyBufferToBuffer_NullEncoder_ReturnsError)
{
    GfxCopyBufferToBufferDescriptor desc = {};
    ASSERT_EQ(gfxCommandEncoderCopyBufferToBuffer(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderCopyBufferToTexture_NullEncoder_ReturnsError)
{
    GfxCopyBufferToTextureDescriptor desc = {};
    ASSERT_EQ(gfxCommandEncoderCopyBufferToTexture(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderCopyTextureToBuffer_NullEncoder_ReturnsError)
{
    GfxCopyTextureToBufferDescriptor desc = {};
    ASSERT_EQ(gfxCommandEncoderCopyTextureToBuffer(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderCopyTextureToTexture_NullEncoder_ReturnsError)
{
    GfxCopyTextureToTextureDescriptor desc = {};
    ASSERT_EQ(gfxCommandEncoderCopyTextureToTexture(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderBlitTextureToTexture_NullEncoder_ReturnsError)
{
    GfxBlitTextureToTextureDescriptor desc = {};
    ASSERT_EQ(gfxCommandEncoderBlitTextureToTexture(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderPipelineBarrier_NullEncoder_ReturnsError)
{
    GfxPipelineBarrierDescriptor desc = {};
    ASSERT_EQ(gfxCommandEncoderPipelineBarrier(nullptr, &desc), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderGenerateMipmaps_NullEncoder_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    ASSERT_EQ(gfxCommandEncoderGenerateMipmaps(nullptr, texture), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderGenerateMipmapsRange_NullEncoder_ReturnsError)
{
    GfxTexture texture = reinterpret_cast<GfxTexture>(0x1);
    ASSERT_EQ(gfxCommandEncoderGenerateMipmapsRange(nullptr, texture, 0, 1), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderWriteTimestamp_NullEncoder_ReturnsError)
{
    GfxQuerySet querySet = reinterpret_cast<GfxQuerySet>(0x1);
    ASSERT_EQ(gfxCommandEncoderWriteTimestamp(nullptr, querySet, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderResolveQuerySet_NullEncoder_ReturnsError)
{
    GfxQuerySet querySet = reinterpret_cast<GfxQuerySet>(0x1);
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxCommandEncoderResolveQuerySet(nullptr, querySet, 0, 1, buffer, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderBeginRenderPass_NullEncoder_ReturnsError)
{
    GfxRenderPassBeginDescriptor desc = {};
    GfxRenderPassEncoder encoder;
    ASSERT_EQ(gfxCommandEncoderBeginRenderPass(nullptr, &desc, &encoder), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, CommandEncoderBeginComputePass_NullEncoder_ReturnsError)
{
    GfxComputePassBeginDescriptor desc = {};
    GfxComputePassEncoder encoder;
    ASSERT_EQ(gfxCommandEncoderBeginComputePass(nullptr, &desc, &encoder), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// RenderPass Encoder Operations
TEST_F(GfxImplTest, RenderPassEncoderSetPipeline_NullEncoder_ReturnsError)
{
    GfxRenderPipeline pipeline = reinterpret_cast<GfxRenderPipeline>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderSetPipeline(nullptr, pipeline), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderSetBindGroup_NullEncoder_ReturnsError)
{
    GfxBindGroup bindGroup = reinterpret_cast<GfxBindGroup>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderSetBindGroup(nullptr, 0, bindGroup, nullptr, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderSetVertexBuffer_NullEncoder_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderSetVertexBuffer(nullptr, 0, buffer, 0, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderSetIndexBuffer_NullEncoder_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderSetIndexBuffer(nullptr, buffer, GFX_INDEX_FORMAT_UINT16, 0, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderSetViewport_NullEncoder_ReturnsError)
{
    GfxViewport viewport = {};
    ASSERT_EQ(gfxRenderPassEncoderSetViewport(nullptr, &viewport), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderSetScissorRect_NullEncoder_ReturnsError)
{
    GfxScissorRect scissor = {};
    ASSERT_EQ(gfxRenderPassEncoderSetScissorRect(nullptr, &scissor), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderDraw_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxRenderPassEncoderDraw(nullptr, 0, 0, 0, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderDrawIndexed_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxRenderPassEncoderDrawIndexed(nullptr, 0, 0, 0, 0, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderDrawIndirect_NullEncoder_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderDrawIndirect(nullptr, buffer, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderDrawIndexedIndirect_NullEncoder_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderDrawIndexedIndirect(nullptr, buffer, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderBeginOcclusionQuery_NullEncoder_ReturnsError)
{
    GfxQuerySet querySet = reinterpret_cast<GfxQuerySet>(0x1);
    ASSERT_EQ(gfxRenderPassEncoderBeginOcclusionQuery(nullptr, querySet, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderEndOcclusionQuery_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxRenderPassEncoderEndOcclusionQuery(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, RenderPassEncoderEnd_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxRenderPassEncoderEnd(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ComputePass Encoder Operations
TEST_F(GfxImplTest, ComputePassEncoderSetPipeline_NullEncoder_ReturnsError)
{
    GfxComputePipeline pipeline = reinterpret_cast<GfxComputePipeline>(0x1);
    ASSERT_EQ(gfxComputePassEncoderSetPipeline(nullptr, pipeline), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, ComputePassEncoderSetBindGroup_NullEncoder_ReturnsError)
{
    GfxBindGroup bindGroup = reinterpret_cast<GfxBindGroup>(0x1);
    ASSERT_EQ(gfxComputePassEncoderSetBindGroup(nullptr, 0, bindGroup, nullptr, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, ComputePassEncoderDispatch_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxComputePassEncoderDispatch(nullptr, 1, 1, 1), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, ComputePassEncoderDispatchIndirect_NullEncoder_ReturnsError)
{
    GfxBuffer buffer = reinterpret_cast<GfxBuffer>(0x1);
    ASSERT_EQ(gfxComputePassEncoderDispatchIndirect(nullptr, buffer, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, ComputePassEncoderEnd_NullEncoder_ReturnsError)
{
    ASSERT_EQ(gfxComputePassEncoderEnd(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Fence Operations
TEST_F(GfxImplTest, FenceGetStatus_NullFence_ReturnsError)
{
    bool signaled;
    ASSERT_EQ(gfxFenceGetStatus(nullptr, &signaled), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, FenceReset_NullFence_ReturnsError)
{
    ASSERT_EQ(gfxFenceReset(nullptr), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Semaphore Operations
TEST_F(GfxImplTest, SemaphoreGetType_NullSemaphore_ReturnsError)
{
    GfxSemaphoreType type;
    ASSERT_EQ(gfxSemaphoreGetType(nullptr, &type), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SemaphoreSignal_NullSemaphore_ReturnsError)
{
    ASSERT_EQ(gfxSemaphoreSignal(nullptr, 1), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SemaphoreWait_NullSemaphore_ReturnsError)
{
    ASSERT_EQ(gfxSemaphoreWait(nullptr, 1, 0), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_F(GfxImplTest, SemaphoreGetValue_NullSemaphore_ReturnsError)
{
    uint64_t value;
    ASSERT_EQ(gfxSemaphoreGetValue(nullptr, &value), GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

} // namespace gfx::backend::test
