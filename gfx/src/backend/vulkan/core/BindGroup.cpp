#include "BindGroup.h"

#include "Device.h"

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace gfx::backend::vulkan::core {

BindGroup::BindGroup(Device* device, const BindGroupCreateInfo& createInfo)
    : m_device(device)
{
    // Count actual descriptors needed by type
    std::unordered_map<VkDescriptorType, uint32_t> descriptorCounts;

    for (const auto& entry : createInfo.entries) {
        ++descriptorCounts[entry.descriptorType];
    }

    // Create descriptor pool with exact sizes
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const auto& [type, count] : descriptorCounts) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = type;
        poolSize.descriptorCount = count;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1; // Each BindGroup only allocates one descriptor set

    VkResult result = vkCreateDescriptorPool(m_device->handle(), &poolInfo, nullptr, &m_pool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    // Allocate descriptor set
    VkDescriptorSetLayout setLayout = createInfo.layout;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &setLayout;

    result = vkAllocateDescriptorSets(m_device->handle(), &allocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    // Update descriptor set
    // Build all the descriptor info arrays first
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    // Reserve space to avoid reallocation and pointer invalidation
    bufferInfos.reserve(createInfo.entries.size());
    imageInfos.reserve(createInfo.entries.size());
    descriptorWrites.reserve(createInfo.entries.size());

    // Track indices for buffer and image infos
    size_t bufferInfoIndex = 0;
    size_t imageInfoIndex = 0;

    for (const auto& entry : createInfo.entries) {
        if (entry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || entry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = entry.buffer;
            bufferInfo.offset = entry.bufferOffset;
            bufferInfo.range = entry.bufferSize;
            bufferInfos.push_back(bufferInfo);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_descriptorSet;
            descriptorWrite.dstBinding = entry.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = entry.descriptorType;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfos[bufferInfoIndex++];

            descriptorWrites.push_back(descriptorWrite);
        } else if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = entry.sampler;
            imageInfo.imageView = VK_NULL_HANDLE;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfos.push_back(imageInfo);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_descriptorSet;
            descriptorWrite.dstBinding = entry.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfos[imageInfoIndex++];

            descriptorWrites.push_back(descriptorWrite);
        } else if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || entry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = VK_NULL_HANDLE;
            imageInfo.imageView = entry.imageView;
            imageInfo.imageLayout = entry.imageLayout;
            imageInfos.push_back(imageInfo);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_descriptorSet;
            descriptorWrite.dstBinding = entry.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = entry.descriptorType;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfos[imageInfoIndex++];

            descriptorWrites.push_back(descriptorWrite);
        }
    }

    if (!descriptorWrites.empty()) {
        vkUpdateDescriptorSets(m_device->handle(), static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(), 0, nullptr);
    }
}

BindGroup::~BindGroup()
{
    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device->handle(), m_pool, nullptr);
    }
}

VkDescriptorSet BindGroup::handle() const
{
    return m_descriptorSet;
}

} // namespace gfx::backend::vulkan::core