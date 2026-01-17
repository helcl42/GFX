#ifndef GFX_VULKAN_SHADER_H
#define GFX_VULKAN_SHADER_H

#include "CoreTypes.h"

#include <string>

namespace gfx::backend::vulkan::core {

class Device;

class Shader {
public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Device* device, const ShaderCreateInfo& createInfo);
    ~Shader();

    VkShaderModule handle() const;
    const char* entryPoint() const;

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    std::string m_entryPoint;
    Device* m_device = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_SHADER_H