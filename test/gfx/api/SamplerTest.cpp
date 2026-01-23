#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxSamplerTest : public testing::TestWithParam<GfxBackend> {
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

TEST_P(GfxSamplerTest, CreateSamplerWithValidDescriptor)
{
    GfxSamplerDescriptor desc = {};
    desc.label = "Test Sampler";
    desc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    desc.magFilter = GFX_FILTER_MODE_LINEAR;
    desc.minFilter = GFX_FILTER_MODE_LINEAR;
    desc.mipmapFilter = GFX_FILTER_MODE_LINEAR;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = GFX_COMPARE_FUNCTION_UNDEFINED;
    desc.maxAnisotropy = 1;

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(device, &desc, &sampler);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(sampler, nullptr);

    if (sampler) {
        gfxSamplerDestroy(sampler);
    }
}

TEST_P(GfxSamplerTest, CreateSamplerWithNullDevice)
{
    GfxSamplerDescriptor desc = {};
    desc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    desc.magFilter = GFX_FILTER_MODE_LINEAR;
    desc.minFilter = GFX_FILTER_MODE_LINEAR;
    desc.mipmapFilter = GFX_FILTER_MODE_LINEAR;

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(nullptr, &desc, &sampler);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSamplerTest, CreateSamplerWithNullDescriptor)
{
    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(device, nullptr, &sampler);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSamplerTest, CreateSamplerWithNullOutput)
{
    GfxSamplerDescriptor desc = {};
    desc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    desc.magFilter = GFX_FILTER_MODE_LINEAR;
    desc.minFilter = GFX_FILTER_MODE_LINEAR;
    desc.mipmapFilter = GFX_FILTER_MODE_LINEAR;

    GfxResult result = gfxDeviceCreateSampler(device, &desc, nullptr);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSamplerTest, CreateSamplerWithClampToEdge)
{
    GfxSamplerDescriptor desc = {};
    desc.label = "Clamp Sampler";
    desc.addressModeU = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeV = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeW = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.magFilter = GFX_FILTER_MODE_LINEAR;
    desc.minFilter = GFX_FILTER_MODE_LINEAR;
    desc.mipmapFilter = GFX_FILTER_MODE_NEAREST;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 0.0f;
    desc.compare = GFX_COMPARE_FUNCTION_UNDEFINED;
    desc.maxAnisotropy = 1;

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(device, &desc, &sampler);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(sampler, nullptr);

    if (sampler) {
        gfxSamplerDestroy(sampler);
    }
}

TEST_P(GfxSamplerTest, CreateSamplerWithNearestFiltering)
{
    GfxSamplerDescriptor desc = {};
    desc.label = "Nearest Sampler";
    desc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    desc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    desc.magFilter = GFX_FILTER_MODE_NEAREST;
    desc.minFilter = GFX_FILTER_MODE_NEAREST;
    desc.mipmapFilter = GFX_FILTER_MODE_NEAREST;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = GFX_COMPARE_FUNCTION_UNDEFINED;
    desc.maxAnisotropy = 1;

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(device, &desc, &sampler);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(sampler, nullptr);

    if (sampler) {
        gfxSamplerDestroy(sampler);
    }
}

TEST_P(GfxSamplerTest, CreateSamplerWithCompareFunction)
{
    GfxSamplerDescriptor desc = {};
    desc.label = "Compare Sampler";
    desc.addressModeU = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeV = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.addressModeW = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    desc.magFilter = GFX_FILTER_MODE_LINEAR;
    desc.minFilter = GFX_FILTER_MODE_LINEAR;
    desc.mipmapFilter = GFX_FILTER_MODE_LINEAR;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = GFX_COMPARE_FUNCTION_LESS_EQUAL; // For shadow mapping
    desc.maxAnisotropy = 1;

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(device, &desc, &sampler);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(sampler, nullptr);

    if (sampler) {
        gfxSamplerDestroy(sampler);
    }
}

TEST_P(GfxSamplerTest, CreateMultipleSamplers)
{
    const int samplerCount = 5;
    GfxSampler samplers[samplerCount] = {};

    for (int i = 0; i < samplerCount; ++i) {
        GfxSamplerDescriptor desc = {};
        desc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
        desc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
        desc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
        desc.magFilter = GFX_FILTER_MODE_LINEAR;
        desc.minFilter = GFX_FILTER_MODE_LINEAR;
        desc.mipmapFilter = GFX_FILTER_MODE_LINEAR;
        desc.lodMinClamp = 0.0f;
        desc.lodMaxClamp = 1000.0f;
        desc.compare = GFX_COMPARE_FUNCTION_UNDEFINED;
        desc.maxAnisotropy = 1;

        GfxResult result = gfxDeviceCreateSampler(device, &desc, &samplers[i]);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(samplers[i], nullptr);
    }

    for (int i = 0; i < samplerCount; ++i) {
        if (samplers[i]) {
            gfxSamplerDestroy(samplers[i]);
        }
    }
}

TEST_P(GfxSamplerTest, DestroySamplerWithNull)
{
    // Destroying NULL should return an error (consistent with other destroy functions)
    GfxResult result = gfxSamplerDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxSamplerTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
