/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: DescSet declaration.
 */

#ifndef VULKANEXAMPLES_DESCSET_H
#define VULKANEXAMPLES_DESCSET_H

#include "NonCopyable.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"

namespace rt {
class DescSet : private NonCopyable {
public:
    explicit DescSet(vks::VulkanDevice *vulkandevice);
    virtual ~DescSet() noexcept;

    bool Init();
    void Update(std::vector<VkWriteDescriptorSet> &writeDescriptorSets) const;
    const VkDescriptorSetLayout GetDescSetLayout() const
    {
        return m_descSetLayout;
    }

    const VkDescriptorSet &GetDescSet() const
    {
        return m_descSet;
    }

protected:
    virtual bool InitLayout() = 0;
    bool InitDescSetPool();
    bool CreateDescSet();

    vks::VulkanDevice *m_vulkanDevice = nullptr;

    VkDescriptorPool m_descSetPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet = VK_NULL_HANDLE;
};
} // namespace rt

#endif // VULKANEXAMPLES_DESCSET_H
