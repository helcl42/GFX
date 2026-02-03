#include <core/sync/Semaphore.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

namespace gfx {

class SemaphoreImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "SemaphoreImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc = GFX_DEVICE_DESCRIPTOR_INIT;

        // Check if timeline semaphore extension is supported
        uint32_t extensionCount = 0;
        gfxAdapterEnumerateExtensions(adapter, &extensionCount, nullptr);
        std::vector<const char*> supportedExtensions(extensionCount);
        gfxAdapterEnumerateExtensions(adapter, &extensionCount, supportedExtensions.data());

        bool timelineSemaphoreSupported = false;
        for (uint32_t i = 0; i < extensionCount; i++) {
            if (strcmp(supportedExtensions[i], GFX_DEVICE_EXTENSION_TIMELINE_SEMAPHORE) == 0) {
                timelineSemaphoreSupported = true;
                break;
            }
        }

        // Enable timeline semaphore extension if supported
        const char* deviceExtensions[] = { GFX_DEVICE_EXTENSION_TIMELINE_SEMAPHORE };
        if (timelineSemaphoreSupported) {
            deviceDesc = {
                .label = nullptr,
                .queueRequests = nullptr,
                .queueRequestCount = 0,
                .enabledExtensions = deviceExtensions,
                .enabledExtensionCount = 1
            };
        }

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

TEST_P(SemaphoreImplTest, CreateAndDestroyBinary)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .label = "Test Binary Semaphore",
        .type = SemaphoreType::Binary
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    EXPECT_NE(semaphore, nullptr);
}

TEST_P(SemaphoreImplTest, CreateAndDestroyTimeline)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .label = "Test Timeline Semaphore",
        .type = SemaphoreType::Timeline,
        .initialValue = 0
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    if (!semaphore) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }
    EXPECT_NE(semaphore, nullptr);
}

TEST_P(SemaphoreImplTest, GetTypeBinary)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .type = SemaphoreType::Binary
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    ASSERT_NE(semaphore, nullptr);

    SemaphoreType type = semaphore->getType();
    EXPECT_EQ(type, SemaphoreType::Binary);
}

TEST_P(SemaphoreImplTest, GetTypeTimeline)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .type = SemaphoreType::Timeline,
        .initialValue = 0
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    if (!semaphore) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    SemaphoreType type = semaphore->getType();
    EXPECT_EQ(type, SemaphoreType::Timeline);
}

TEST_P(SemaphoreImplTest, TimelineInitialValue)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .type = SemaphoreType::Timeline,
        .initialValue = 42
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    if (!semaphore) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    uint64_t value = semaphore->getValue();
    EXPECT_EQ(value, 42);
}

TEST_P(SemaphoreImplTest, TimelineSignal)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .type = SemaphoreType::Timeline,
        .initialValue = 0
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    if (!semaphore) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    semaphore->signal(10);

    uint64_t value = semaphore->getValue();
    EXPECT_EQ(value, 10);
}

TEST_P(SemaphoreImplTest, TimelineWait)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .type = SemaphoreType::Timeline,
        .initialValue = 5
    };

    auto semaphore = deviceWrapper.createSemaphore(semaphoreDesc);
    if (!semaphore) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    // Should succeed immediately since value is already 5
    bool success = semaphore->wait(5, 0);
    EXPECT_TRUE(success);
}

TEST_P(SemaphoreImplTest, MultipleSemaphores_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    SemaphoreDescriptor semaphoreDesc{
        .type = SemaphoreType::Binary
    };

    auto semaphore1 = deviceWrapper.createSemaphore(semaphoreDesc);
    auto semaphore2 = deviceWrapper.createSemaphore(semaphoreDesc);

    EXPECT_NE(semaphore1, nullptr);
    EXPECT_NE(semaphore2, nullptr);
    EXPECT_NE(semaphore1, semaphore2);
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, SemaphoreImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, SemaphoreImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, SemaphoreImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
