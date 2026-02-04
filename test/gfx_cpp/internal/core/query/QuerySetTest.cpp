#include <core/query/QuerySet.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class QuerySetImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "QuerySetImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR,
            .pNext = nullptr,
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{
            .sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR,
            .pNext = nullptr,
            .label = nullptr,
            .queueRequests = nullptr,
            .queueRequestCount = 0,
            .enabledExtensions = nullptr,
            .enabledExtensionCount = 0
        };
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);
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

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

TEST_P(QuerySetImplTest, CreateOcclusionQuerySet)
{
    DeviceImpl deviceWrapper(device);

    QuerySetDescriptor desc{
        .type = QueryType::Occlusion,
        .count = 4
    };

    auto querySet = deviceWrapper.createQuerySet(desc);
    EXPECT_NE(querySet, nullptr);
}

TEST_P(QuerySetImplTest, CreateTimestampQuerySet)
{
    DeviceImpl deviceWrapper(device);

    QuerySetDescriptor desc{
        .type = QueryType::Timestamp,
        .count = 2
    };

    auto querySet = deviceWrapper.createQuerySet(desc);
    EXPECT_NE(querySet, nullptr);
}

TEST_P(QuerySetImplTest, MultipleQuerySets_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    QuerySetDescriptor desc{
        .type = QueryType::Occlusion,
        .count = 4
    };

    auto querySet1 = deviceWrapper.createQuerySet(desc);
    auto querySet2 = deviceWrapper.createQuerySet(desc);

    EXPECT_NE(querySet1, nullptr);
    EXPECT_NE(querySet2, nullptr);
    EXPECT_NE(querySet1, querySet2);
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, QuerySetImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, QuerySetImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, QuerySetImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
