#include <backend/webgpu/core/query/QuerySet.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUQuerySetTest : public testing::Test {
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

TEST_F(WebGPUQuerySetTest, CreateQuerySet_Occlusion)
{
    gfx::backend::webgpu::core::QuerySetCreateInfo createInfo{};
    createInfo.type = WGPUQueryType_Occlusion;
    createInfo.count = 16;

    auto querySet = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo);

    EXPECT_NE(querySet->handle(), nullptr);
    EXPECT_EQ(querySet->getType(), WGPUQueryType_Occlusion);
    EXPECT_EQ(querySet->getCount(), 16);
}

TEST_F(WebGPUQuerySetTest, Handle_ReturnsValidWGPUQuerySet)
{
    gfx::backend::webgpu::core::QuerySetCreateInfo createInfo{};
    createInfo.type = WGPUQueryType_Occlusion;
    createInfo.count = 8;

    auto querySet = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo);

    WGPUQuerySet handle = querySet->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUQuerySetTest, GetDevice_ReturnsCorrectDevice)
{
    gfx::backend::webgpu::core::QuerySetCreateInfo createInfo{};
    createInfo.type = WGPUQueryType_Occlusion;
    createInfo.count = 4;

    auto querySet = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo);

    EXPECT_EQ(querySet->getDevice(), device.get());
}

TEST_F(WebGPUQuerySetTest, CreateQuerySet_Timestamp)
{
    gfx::backend::webgpu::core::QuerySetCreateInfo createInfo{};
    createInfo.type = WGPUQueryType_Timestamp;
    createInfo.count = 32;

    auto querySet = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo);

    EXPECT_NE(querySet->handle(), nullptr);
    EXPECT_EQ(querySet->getType(), WGPUQueryType_Timestamp);
    EXPECT_EQ(querySet->getCount(), 32);
}

TEST_F(WebGPUQuerySetTest, MultipleQuerySets_CanCoexist)
{
    gfx::backend::webgpu::core::QuerySetCreateInfo createInfo1{};
    createInfo1.type = WGPUQueryType_Occlusion;
    createInfo1.count = 8;

    gfx::backend::webgpu::core::QuerySetCreateInfo createInfo2{};
    createInfo2.type = WGPUQueryType_Timestamp;
    createInfo2.count = 16;

    auto querySet1 = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo1);
    auto querySet2 = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo2);

    EXPECT_NE(querySet1->handle(), nullptr);
    EXPECT_NE(querySet2->handle(), nullptr);
    EXPECT_NE(querySet1->handle(), querySet2->handle());

    EXPECT_EQ(querySet1->getType(), WGPUQueryType_Occlusion);
    EXPECT_EQ(querySet2->getType(), WGPUQueryType_Timestamp);
}

TEST_F(WebGPUQuerySetTest, Destructor_CleansUpResources)
{
    {
        gfx::backend::webgpu::core::QuerySetCreateInfo createInfo{};
        createInfo.type = WGPUQueryType_Occlusion;
        createInfo.count = 64;

        auto querySet = std::make_unique<gfx::backend::webgpu::core::QuerySet>(device.get(), createInfo);
        EXPECT_NE(querySet->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
