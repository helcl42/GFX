#include "Shader.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

Shader::Shader(Device* device, const ShaderCreateInfo& createInfo)
    : m_device(device)
{
    if (createInfo.entryPoint) {
        m_entryPoint = createInfo.entryPoint;
    } else {
        m_entryPoint = "main";
    }

    VkShaderModuleCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkCreateInfo.codeSize = createInfo.codeSize;
    vkCreateInfo.pCode = reinterpret_cast<const uint32_t*>(createInfo.code);

    VkResult result = vkCreateShaderModule(m_device->handle(), &vkCreateInfo, nullptr, &m_shaderModule);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
}

Shader::~Shader()
{
    if (m_shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_device->handle(), m_shaderModule, nullptr);
    }
}

VkShaderModule Shader::handle() const
{
    return m_shaderModule;
}

const char* Shader::entryPoint() const
{
    return m_entryPoint.c_str();
}

} // namespace gfx::backend::vulkan::core