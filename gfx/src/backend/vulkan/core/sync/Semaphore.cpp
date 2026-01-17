#include "Semaphore.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

Semaphore::Semaphore(Device* device, const SemaphoreCreateInfo& createInfo)
    : m_device(device)
    , m_type(createInfo.type)
{

    if (m_type == SemaphoreType::Timeline) {
        // Timeline semaphore
        VkSemaphoreTypeCreateInfo timelineInfo{};
        timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineInfo.initialValue = createInfo.initialValue;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.pNext = &timelineInfo;

        VkResult result = vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_semaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create timeline semaphore");
        }
    } else {
        // Binary semaphore
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult result = vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_semaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create binary semaphore");
        }
    }
}

Semaphore::~Semaphore()
{
    if (m_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_device->handle(), m_semaphore, nullptr);
    }
}

VkSemaphore Semaphore::handle() const
{
    return m_semaphore;
}

SemaphoreType Semaphore::getType() const
{
    return m_type;
}

VkResult Semaphore::signal(uint64_t value)
{
    if (m_type != SemaphoreType::Timeline) {
        return VK_ERROR_VALIDATION_FAILED_EXT; // Binary semaphores can't be manually signaled
    }

    VkSemaphoreSignalInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    signalInfo.semaphore = m_semaphore;
    signalInfo.value = value;

    return vkSignalSemaphore(m_device->handle(), &signalInfo);
}

VkResult Semaphore::wait(uint64_t value, uint64_t timeoutNs)
{
    if (m_type != SemaphoreType::Timeline) {
        return VK_ERROR_VALIDATION_FAILED_EXT; // Binary semaphores can't be manually waited
    }

    VkSemaphoreWaitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &m_semaphore;
    waitInfo.pValues = &value;

    return vkWaitSemaphores(m_device->handle(), &waitInfo, timeoutNs);
}

uint64_t Semaphore::getValue() const
{
    if (m_type != SemaphoreType::Timeline) {
        return 0; // Binary semaphores don't have values
    }

    uint64_t value = 0;
    vkGetSemaphoreCounterValue(m_device->handle(), m_semaphore, &value);
    return value;
}

} // namespace gfx::backend::vulkan::core