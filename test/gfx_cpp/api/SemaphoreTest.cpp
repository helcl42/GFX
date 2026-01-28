#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxCppSemaphoreTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            instance = gfx::createInstance({ .backend = backend, .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG } });
            adapter = instance->requestAdapter({ .adapterIndex = 0 });

            // Check if timeline semaphore extension is supported
            auto supportedExtensions = adapter->enumerateExtensions();
            bool timelineSemaphoreSupported = std::find(supportedExtensions.begin(), supportedExtensions.end(),
                                                  std::string(gfx::DEVICE_EXTENSION_TIMELINE_SEMAPHORE))
                != supportedExtensions.end();

            // Enable timeline semaphore extension if supported
            std::vector<std::string> deviceExtensions;
            if (timelineSemaphoreSupported) {
                deviceExtensions.push_back(gfx::DEVICE_EXTENSION_TIMELINE_SEMAPHORE);
            }

            device = adapter->createDevice({ .label = "Test Device",
                .enabledExtensions = deviceExtensions });
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend = gfx::Backend::Vulkan;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

TEST_P(GfxCppSemaphoreTest, CreateAndDestroyBinary)
{
    auto semaphore = device->createSemaphore({ .label = "Binary Semaphore",
        .type = gfx::SemaphoreType::Binary });

    EXPECT_NE(semaphore, nullptr);
    EXPECT_EQ(semaphore->getType(), gfx::SemaphoreType::Binary);
}

TEST_P(GfxCppSemaphoreTest, CreateAndDestroyTimeline)
{
    try {
        auto semaphore = device->createSemaphore({ .label = "Timeline Semaphore",
            .type = gfx::SemaphoreType::Timeline,
            .initialValue = 0 });

        EXPECT_NE(semaphore, nullptr);
        EXPECT_EQ(semaphore->getType(), gfx::SemaphoreType::Timeline);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Timeline semaphores not supported: " << e.what();
    }
}

TEST_P(GfxCppSemaphoreTest, GetTypeBinary)
{
    auto semaphore = device->createSemaphore({ .type = gfx::SemaphoreType::Binary });

    EXPECT_EQ(semaphore->getType(), gfx::SemaphoreType::Binary);
}

TEST_P(GfxCppSemaphoreTest, GetTypeTimeline)
{
    try {
        auto semaphore = device->createSemaphore({ .type = gfx::SemaphoreType::Timeline,
            .initialValue = 0 });

        EXPECT_EQ(semaphore->getType(), gfx::SemaphoreType::Timeline);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Timeline semaphores not supported: " << e.what();
    }
}

TEST_P(GfxCppSemaphoreTest, TimelineInitialValue)
{
    try {
        auto semaphore = device->createSemaphore({ .type = gfx::SemaphoreType::Timeline,
            .initialValue = 42 });

        EXPECT_EQ(semaphore->getValue(), 42);

        auto semaphore2 = device->createSemaphore({ .type = gfx::SemaphoreType::Timeline,
            .initialValue = 0 });

        EXPECT_EQ(semaphore2->getValue(), 0);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Timeline semaphores not supported: " << e.what();
    }
}

TEST_P(GfxCppSemaphoreTest, TimelineSignal)
{
    try {
        auto semaphore = device->createSemaphore({ .type = gfx::SemaphoreType::Timeline,
            .initialValue = 0 });

        EXPECT_EQ(semaphore->getValue(), 0);

        semaphore->signal(1);
        EXPECT_EQ(semaphore->getValue(), 1);

        semaphore->signal(5);
        EXPECT_EQ(semaphore->getValue(), 5);

        semaphore->signal(100);
        EXPECT_EQ(semaphore->getValue(), 100);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Timeline semaphores not supported: " << e.what();
    }
}

TEST_P(GfxCppSemaphoreTest, TimelineWait)
{
    try {
        auto semaphore = device->createSemaphore({ .type = gfx::SemaphoreType::Timeline,
            .initialValue = 0 });

        // Signal to value 10
        semaphore->signal(10);
        EXPECT_EQ(semaphore->getValue(), 10);

        // Wait for value that's already reached - should return immediately
        bool result = semaphore->wait(5, 1000000000); // 1 second timeout
        EXPECT_TRUE(result);

        // Wait for current value
        result = semaphore->wait(10, 1000000000);
        EXPECT_TRUE(result);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Timeline semaphores not supported: " << e.what();
    }
}

TEST_P(GfxCppSemaphoreTest, CreateWithDefaultDescriptor)
{
    // Default should be binary semaphore
    auto semaphore = device->createSemaphore({});

    EXPECT_NE(semaphore, nullptr);
    EXPECT_EQ(semaphore->getType(), gfx::SemaphoreType::Binary);
}

TEST_P(GfxCppSemaphoreTest, CreateMultipleSemaphores)
{
    const int count = 5;
    std::vector<std::shared_ptr<gfx::Semaphore>> semaphores;

    for (int i = 0; i < count; ++i) {
        auto semaphore = device->createSemaphore({ .type = gfx::SemaphoreType::Binary });

        EXPECT_NE(semaphore, nullptr);
        EXPECT_EQ(semaphore->getType(), gfx::SemaphoreType::Binary);
        semaphores.push_back(semaphore);
    }

    EXPECT_EQ(semaphores.size(), count);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppSemaphoreTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });
