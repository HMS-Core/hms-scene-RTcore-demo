/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Vulkan Pipeline implementation.
 */

#include "Pipeline.h"

namespace rt {
Pipeline::Pipeline(vks::VulkanDevice *vulkanDevice) : m_vulkanDevice(vulkanDevice)
{
}

Pipeline::~Pipeline() noexcept
{
    if (m_vulkanDevice == nullptr || m_vulkanDevice->logicalDevice == VK_NULL_HANDLE) {
        return;
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_vulkanDevice->logicalDevice, m_pipeline, nullptr);
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_vulkanDevice->logicalDevice, m_pipelineLayout, nullptr);
    }
}
} // namespace rt
