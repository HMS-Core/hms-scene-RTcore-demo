/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: DescSet implementation.
 */

#include "DescSet.h"

#include "Log.h"

namespace rt {
namespace Detail {
constexpr int MAX_TEXTURE_NUM = 32;
constexpr int MAX_SSBO_NUM = 16;
constexpr int MAX_UNIFROM_NUM = 16;

inline VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
{
    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type = type;
    descriptorPoolSize.descriptorCount = descriptorCount;
    return descriptorPoolSize;
}
} // namespace Detail

DescSet::DescSet(vks::VulkanDevice *vulkandevice) : m_vulkanDevice(vulkandevice)
{
}

DescSet::~DescSet() noexcept
{
    if (m_vulkanDevice == nullptr || m_vulkanDevice->logicalDevice == VK_NULL_HANDLE) {
        return;
    }

    if (m_descSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_vulkanDevice->logicalDevice, m_descSetPool, 1, &m_descSet);
    }

    if (m_descSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_vulkanDevice->logicalDevice, m_descSetLayout, nullptr);
    }

    if (m_descSetPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_vulkanDevice->logicalDevice, m_descSetPool, nullptr);
    }
}

bool DescSet::Init()
{
    if (m_vulkanDevice == nullptr || m_vulkanDevice->logicalDevice == VK_NULL_HANDLE) {
        return false;
    }

    if (!InitDescSetPool()) {
        return false;
    }

    if (!InitLayout()) {
        return false;
    }

    if (!CreateDescSet()) {
        return false;
    }

    return true;
}

bool DescSet::InitDescSetPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        Detail::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Detail::MAX_TEXTURE_NUM),
        Detail::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Detail::MAX_SSBO_NUM),
        Detail::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Detail::MAX_UNIFROM_NUM)};

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = 1;

    VkResult res = vkCreateDescriptorPool(m_vulkanDevice->logicalDevice, &descriptorPoolInfo, nullptr, &m_descSetPool);
    if (res != VK_SUCCESS) {
        LOGE("%s: Failed to create DescriptorPool, err: %d", __func__, static_cast<int>(res));
        return false;
    }

    return true;
}

bool DescSet::CreateDescSet()
{
    VkDescriptorSetAllocateInfo decSetAllocInfo{};
    decSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    decSetAllocInfo.descriptorPool = m_descSetPool;
    decSetAllocInfo.pSetLayouts = &m_descSetLayout;
    decSetAllocInfo.descriptorSetCount = 1;
    VkResult res = vkAllocateDescriptorSets(m_vulkanDevice->logicalDevice, &decSetAllocInfo, &m_descSet);
    if (res != VK_SUCCESS) {
        m_descSet = VK_NULL_HANDLE;
        LOGE("%s: Failed to create DescriptorSet, err: %d", __func__, static_cast<int>(res));
        return false;
    }

    return true;
}

void DescSet::Update(std::vector<VkWriteDescriptorSet> &writeDescriptorSets) const
{
    ASSERT(m_descSet != VK_NULL_HANDLE);

    for (auto &writeDescriptorSet : writeDescriptorSets) {
        writeDescriptorSet.dstSet = m_descSet;
    }

    vkUpdateDescriptorSets(
        m_vulkanDevice->logicalDevice,
        static_cast<uint32_t>(writeDescriptorSets.size()),
        writeDescriptorSets.data(),
        0,
        nullptr);
}
} // namespace rt
