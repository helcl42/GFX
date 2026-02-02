#include <backend/vulkan/core/render/RenderPass.h>
#include <backend/vulkan/core/render/RenderPipeline.h>
#include <backend/vulkan/core/resource/Shader.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core RenderPipeline class
// These tests verify the internal render pipeline implementation

namespace {

// ============================================================================
// Minimal valid SPIR-V shader modules for testing
// ============================================================================

// Minimal vertex shader SPIR-V compiled from:
// #version 450
// layout(location = 0) out vec3 fragColor;
// void main() {
//     gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
//     fragColor = vec3(1.0, 0.0, 0.0);
// }
static const uint32_t MINIMAL_VERTEX_SPIRV[] = {
    0x07230203, 0x00010000, 0x000d000b, 0x00000019, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000017, 0x00030003,
    0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79,
    0x656e696c, 0x7269645f, 0x69746365, 0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45,
    0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
    0x00060005, 0x0000000b, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000b,
    0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x0000000b, 0x00000001, 0x505f6c67,
    0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000000b, 0x00000002, 0x435f6c67, 0x4470696c,
    0x61747369, 0x0065636e, 0x00070006, 0x0000000b, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369,
    0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00050005, 0x00000017, 0x67617266, 0x6f6c6f43,
    0x00000072, 0x00030047, 0x0000000b, 0x00000002, 0x00050048, 0x0000000b, 0x00000000, 0x0000000b,
    0x00000000, 0x00050048, 0x0000000b, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000000b,
    0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003, 0x0000000b, 0x00000004,
    0x00040047, 0x00000017, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004,
    0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000001,
    0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b, 0x00000007, 0x00000006,
    0x0000000a, 0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b, 0x0004003b, 0x0000000c,
    0x0000000d, 0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e,
    0x0000000f, 0x00000000, 0x0004002b, 0x00000006, 0x00000010, 0x00000000, 0x0004002b, 0x00000006,
    0x00000011, 0x3f800000, 0x0007002c, 0x00000007, 0x00000012, 0x00000010, 0x00000010, 0x00000010,
    0x00000011, 0x00040020, 0x00000013, 0x00000003, 0x00000007, 0x00040017, 0x00000015, 0x00000006,
    0x00000003, 0x00040020, 0x00000016, 0x00000003, 0x00000015, 0x0004003b, 0x00000016, 0x00000017,
    0x00000003, 0x0006002c, 0x00000015, 0x00000018, 0x00000011, 0x00000010, 0x00000010, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000013,
    0x00000014, 0x0000000d, 0x0000000f, 0x0003003e, 0x00000014, 0x00000012, 0x0003003e, 0x00000017,
    0x00000018, 0x000100fd, 0x00010038
};

// Minimal fragment shader SPIR-V compiled from:
// #version 450
// layout(location = 0) in vec3 fragColor;
// layout(location = 0) out vec4 outColor;
// void main() {
//     outColor = vec4(fragColor, 1.0);
// }
static const uint32_t MINIMAL_FRAGMENT_SPIRV[] = {
    0x07230203, 0x00010000, 0x000d000b, 0x00000013, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f,
    0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365, 0x00006576, 0x00080004,
    0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
    0x00050005, 0x0000000c, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009, 0x0000001e,
    0x00000000, 0x00040047, 0x0000000c, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021,
    0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
    0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009,
    0x00000003, 0x00040017, 0x0000000a, 0x00000006, 0x00000003, 0x00040020, 0x0000000b, 0x00000001,
    0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000001, 0x0004002b, 0x00000006, 0x0000000e,
    0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
    0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c, 0x00050051, 0x00000006, 0x0000000f, 0x0000000d,
    0x00000000, 0x00050051, 0x00000006, 0x00000010, 0x0000000d, 0x00000001, 0x00050051, 0x00000006,
    0x00000011, 0x0000000d, 0x00000002, 0x00070050, 0x00000007, 0x00000012, 0x0000000f, 0x00000010,
    0x00000011, 0x0000000e, 0x0003003e, 0x00000009, 0x00000012, 0x000100fd, 0x00010038
};

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanRenderPipelineTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::vulkan::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::vulkan::core::Instance>(instInfo);

            gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::vulkan::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, deviceInfo);

            // Create shaders for reuse
            gfx::backend::vulkan::core::ShaderCreateInfo vertShaderInfo{};
            vertShaderInfo.code = MINIMAL_VERTEX_SPIRV;
            vertShaderInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
            vertShaderInfo.entryPoint = "main";
            vertexShader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), vertShaderInfo);

            gfx::backend::vulkan::core::ShaderCreateInfo fragShaderInfo{};
            fragShaderInfo.code = MINIMAL_FRAGMENT_SPIRV;
            fragShaderInfo.codeSize = sizeof(MINIMAL_FRAGMENT_SPIRV);
            fragShaderInfo.entryPoint = "main";
            fragmentShader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), fragShaderInfo);

            // Create a simple render pass for reuse
            gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
            gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
            colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
            colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
            colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            rpInfo.colorAttachments.push_back(colorAtt);
            renderPass = std::make_unique<gfx::backend::vulkan::core::RenderPass>(device.get(), rpInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
    std::unique_ptr<gfx::backend::vulkan::core::Shader> vertexShader;
    std::unique_ptr<gfx::backend::vulkan::core::Shader> fragmentShader;
    std::unique_ptr<gfx::backend::vulkan::core::RenderPass> renderPass;
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, CreateMinimalPipeline_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = renderPass->handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline.layout(), VK_NULL_HANDLE);
}

TEST_F(VulkanRenderPipelineTest, CreateWithVertexInput_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = renderPass->handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    // Add vertex buffer layout
    gfx::backend::vulkan::core::VertexBufferLayout vertexBuffer{};
    vertexBuffer.arrayStride = 12; // 3 floats
    vertexBuffer.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attr{};
    attr.binding = 0;
    attr.location = 0;
    attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    attr.offset = 0;
    vertexBuffer.attributes.push_back(attr);

    createInfo.vertex.buffers.push_back(vertexBuffer);

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanRenderPipelineTest, CreateWithDepthStencil_CreatesSuccessfully)
{
    // Create render pass with depth
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.target.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    rpInfo.depthStencilAttachment = depthAtt;

    gfx::backend::vulkan::core::RenderPass depthRenderPass(device.get(), rpInfo);

    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = depthRenderPass.handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    gfx::backend::vulkan::core::DepthStencilState depthState{};
    depthState.format = VK_FORMAT_D32_SFLOAT;
    depthState.depthWriteEnabled = true;
    depthState.depthCompareOp = VK_COMPARE_OP_LESS;
    createInfo.depthStencil = depthState;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Topology Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, DifferentTopologies_CreateSuccessfully)
{
    VkPrimitiveTopology topologies[] = {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
    };

    for (auto topology : topologies) {
        gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
        createInfo.renderPass = renderPass->handle();

        createInfo.vertex.module = vertexShader->handle();
        createInfo.vertex.entryPoint = "main";

        createInfo.fragment.module = fragmentShader->handle();
        createInfo.fragment.entryPoint = "main";

        gfx::backend::vulkan::core::ColorTargetState colorTarget{};
        colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorTarget.blendState.blendEnable = VK_FALSE;
        createInfo.fragment.targets.push_back(colorTarget);

        createInfo.primitive.topology = topology;
        createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
        createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
        createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

        gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

        EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
    }
}

// ============================================================================
// Cull Mode Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, DifferentCullModes_CreateSuccessfully)
{
    VkCullModeFlags cullModes[] = {
        VK_CULL_MODE_NONE,
        VK_CULL_MODE_FRONT_BIT,
        VK_CULL_MODE_BACK_BIT,
        VK_CULL_MODE_FRONT_AND_BACK
    };

    for (auto cullMode : cullModes) {
        gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
        createInfo.renderPass = renderPass->handle();

        createInfo.vertex.module = vertexShader->handle();
        createInfo.vertex.entryPoint = "main";

        createInfo.fragment.module = fragmentShader->handle();
        createInfo.fragment.entryPoint = "main";

        gfx::backend::vulkan::core::ColorTargetState colorTarget{};
        colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorTarget.blendState.blendEnable = VK_FALSE;
        createInfo.fragment.targets.push_back(colorTarget);

        createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
        createInfo.primitive.cullMode = cullMode;
        createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

        gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

        EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
    }
}

// ============================================================================
// Front Face Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, DifferentFrontFaces_CreateSuccessfully)
{
    VkFrontFace frontFaces[] = {
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        VK_FRONT_FACE_CLOCKWISE
    };

    for (auto frontFace : frontFaces) {
        gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
        createInfo.renderPass = renderPass->handle();

        createInfo.vertex.module = vertexShader->handle();
        createInfo.vertex.entryPoint = "main";

        createInfo.fragment.module = fragmentShader->handle();
        createInfo.fragment.entryPoint = "main";

        gfx::backend::vulkan::core::ColorTargetState colorTarget{};
        colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorTarget.blendState.blendEnable = VK_FALSE;
        createInfo.fragment.targets.push_back(colorTarget);

        createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
        createInfo.primitive.cullMode = VK_CULL_MODE_BACK_BIT;
        createInfo.primitive.frontFace = frontFace;

        createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

        gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

        EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
    }
}

// ============================================================================
// Blend State Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, WithBlending_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = renderPass->handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_TRUE;
    colorTarget.blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorTarget.blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blendState.colorBlendOp = VK_BLEND_OP_ADD;
    colorTarget.blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorTarget.blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorTarget.blendState.alphaBlendOp = VK_BLEND_OP_ADD;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// MSAA Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, WithMSAA_CreatesSuccessfully)
{
    // Create MSAA render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_4_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);
    gfx::backend::vulkan::core::RenderPass msaaRenderPass(device.get(), rpInfo);

    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = msaaRenderPass.handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_4_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = renderPass->handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    VkPipeline handle = pipeline.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
    EXPECT_EQ(pipeline.handle(), handle);
}

TEST_F(VulkanRenderPipelineTest, GetLayout_ReturnsValidLayout)
{
    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = renderPass->handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

    VkPipelineLayout layout = pipeline.layout();
    EXPECT_NE(layout, VK_NULL_HANDLE);
    EXPECT_EQ(pipeline.layout(), layout);
}

TEST_F(VulkanRenderPipelineTest, MultiplePipelines_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
    createInfo.renderPass = renderPass->handle();

    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    createInfo.fragment.module = fragmentShader->handle();
    createInfo.fragment.entryPoint = "main";

    gfx::backend::vulkan::core::ColorTargetState colorTarget{};
    colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorTarget.blendState.blendEnable = VK_FALSE;
    createInfo.fragment.targets.push_back(colorTarget);

    createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
    createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
    createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

    gfx::backend::vulkan::core::RenderPipeline pipeline1(device.get(), createInfo);
    gfx::backend::vulkan::core::RenderPipeline pipeline2(device.get(), createInfo);

    EXPECT_NE(pipeline1.handle(), pipeline2.handle());
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanRenderPipelineTest, CreateAndDestroy_WorksCorrectly)
{
    {
        gfx::backend::vulkan::core::RenderPipelineCreateInfo createInfo{};
        createInfo.renderPass = renderPass->handle();

        createInfo.vertex.module = vertexShader->handle();
        createInfo.vertex.entryPoint = "main";

        createInfo.fragment.module = fragmentShader->handle();
        createInfo.fragment.entryPoint = "main";

        gfx::backend::vulkan::core::ColorTargetState colorTarget{};
        colorTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorTarget.writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorTarget.blendState.blendEnable = VK_FALSE;
        createInfo.fragment.targets.push_back(colorTarget);

        createInfo.primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo.primitive.polygonMode = VK_POLYGON_MODE_FILL;
        createInfo.primitive.cullMode = VK_CULL_MODE_NONE;
        createInfo.primitive.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

        gfx::backend::vulkan::core::RenderPipeline pipeline(device.get(), createInfo);

        EXPECT_NE(pipeline.handle(), VK_NULL_HANDLE);
    }
    // Pipeline destroyed, no crash
}

} // namespace
