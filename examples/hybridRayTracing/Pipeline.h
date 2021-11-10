/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Vulkan Pipeline declaration.
 */

#ifndef VULKANEXAMPLES_PIPELINE_H
#define VULKANEXAMPLES_PIPELINE_H

#include "NonCopyable.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"

namespace rt {
class Pipeline : private NonCopyable {
public:
    explicit Pipeline(vks::VulkanDevice *vulkanDevice);

    // not virtual because we do not use it in cpp's polymorphism way, just use its derived classes.
    ~Pipeline() noexcept;

    const VkPipeline Handle() const
    {
        return m_pipeline;
    }
    const VkPipelineLayout GetPipelineLayout() const
    {
        return m_pipelineLayout;
    }

protected:
    vks::VulkanDevice *m_vulkanDevice = nullptr;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};
} // namespace rt

#endif // VULKANEXAMPLES_PIPELINE_H
