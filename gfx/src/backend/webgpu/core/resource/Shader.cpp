#include "Shader.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

Shader::Shader(Device* device, const ShaderCreateInfo& createInfo)
    : m_device(device)
{
    WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;

    if (createInfo.sourceType == ShaderSourceType::SPIRV) {
        // Use SPIR-V (Dawn extension)
        WGPUShaderSourceSPIRV spirvDesc = WGPU_SHADER_SOURCE_SPIRV_INIT;
        spirvDesc.codeSize = createInfo.codeSize / 4; // Size in uint32_t words
        spirvDesc.code = static_cast<const uint32_t*>(createInfo.code);
        desc.nextInChain = &spirvDesc.chain;

        m_module = wgpuDeviceCreateShaderModule(m_device->handle(), &desc);
    } else {
        // Use WGSL
        WGPUShaderSourceWGSL wgslDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
        // codeSize may include null terminator, but WGPUStringView expects length without it
        size_t codeLength = createInfo.codeSize;
        const char* codeStr = static_cast<const char*>(createInfo.code);
        // If last char is null terminator, exclude it from length
        if (codeLength > 0 && codeStr[codeLength - 1] == '\0') {
            codeLength--;
        }
        wgslDesc.code = { codeStr, codeLength };
        desc.nextInChain = &wgslDesc.chain;

        m_module = wgpuDeviceCreateShaderModule(m_device->handle(), &desc);
    }

    if (!m_module) {
        throw std::runtime_error("Failed to create WebGPU shader module");
    }
}

Shader::~Shader()
{
    if (m_module) {
        wgpuShaderModuleRelease(m_module);
    }
}

WGPUShaderModule Shader::handle() const
{
    return m_module;
}

} // namespace gfx::backend::webgpu::core