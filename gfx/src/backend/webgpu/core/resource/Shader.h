#ifndef GFX_WEBGPU_SHADER_H
#define GFX_WEBGPU_SHADER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class Shader {
public:
    // Prevent copying
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Device* device, const ShaderCreateInfo& createInfo);
    ~Shader();

    WGPUShaderModule handle() const;

private:
    WGPUShaderModule m_module = nullptr;
    Device* m_device = nullptr; // Non-owning pointer for device operations
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_SHADER_H