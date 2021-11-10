/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Utils header.
 */

#ifndef VULKANEXAMPLES_UTILS_H
#define VULKANEXAMPLES_UTILS_H

#include <string>
#include <vulkan/vulkan.h>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <vulkan/vulkan_android.h>
#endif

#include "SaschaWillemsVulkan/VulkanInitializers.hpp"
#include "NonCopyable.h"

namespace Utils {
#ifdef __ANDROID__
std::string GetFileContent(const std::string &fileName, AAssetManager *assetManager);
#else
std::string GetFileContent(const std::string &fileName);
#endif

class ScopedShaderModule : private NonCopyable {
public:
    ScopedShaderModule(VkDevice device, VkShaderModule shaderModule) : m_device(device), m_shaderModule(shaderModule) {}
    ~ScopedShaderModule() noexcept
    {
        if (m_device == VK_NULL_HANDLE) {
            return;
        }

        if (m_shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
            m_shaderModule = VK_NULL_HANDLE;
        }
    }

    bool Valid() const
    {
        return m_shaderModule != VK_NULL_HANDLE;
    }

    const VkShaderModule &Get() const
    {
        return m_shaderModule;
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
};

inline VkPipelineShaderStageCreateInfo PipelineShaderStageInfo(VkShaderModule shaderModule,
                                                               VkShaderStageFlagBits stage,
                                                               const std::string &shaderName)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
    shaderStage.module = shaderModule;
    shaderStage.pName = shaderName.c_str();
    return shaderStage;
}

inline VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorType type,
											   uint32_t binding,
                                               const VkDescriptorImageInfo &imageInfo,
                                               VkDescriptorSet dstSet = VK_NULL_HANDLE,
											   uint32_t descriptorCount = 1)
{
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pImageInfo = &imageInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

inline VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorType type,
											   uint32_t binding,
                                               const VkDescriptorBufferInfo &bufferInfo,
                                               VkDescriptorSet dstSet = VK_NULL_HANDLE,
											   uint32_t descriptorCount = 1)
{
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pBufferInfo = &bufferInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

struct BarrierInfo {
    uint32_t srcQueueIdx;
    uint32_t dstQueueIdx;
    VkAccessFlags srcMask;
    VkAccessFlags dstMask;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
};

inline void SetMemoryBarrier(VkCommandBuffer cmdBuffer, const BarrierInfo &barrierInfo)
{
    VkMemoryBarrier memoryBarrier {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = barrierInfo.srcMask;
    memoryBarrier.dstAccessMask = barrierInfo.dstMask;

    vkCmdPipelineBarrier(cmdBuffer, barrierInfo.srcStageMask, barrierInfo.dstStageMask,
        0, 1, &memoryBarrier, 0, 0, 0, nullptr);
}

inline void BufferBarrier(VkCommandBuffer cmdBuffer, VkBuffer buffer, const BarrierInfo &barrierInfo)
{
    VkBufferMemoryBarrier bufferMemoryBarrier = vks::initializers::bufferMemoryBarrier();
    bufferMemoryBarrier.srcQueueFamilyIndex = barrierInfo.srcQueueIdx;
    bufferMemoryBarrier.dstQueueFamilyIndex = barrierInfo.dstQueueIdx;
    bufferMemoryBarrier.size = VK_WHOLE_SIZE;
    bufferMemoryBarrier.srcAccessMask = barrierInfo.srcMask;
    bufferMemoryBarrier.dstAccessMask = barrierInfo.dstMask;
    bufferMemoryBarrier.buffer = buffer;
    vkCmdPipelineBarrier(cmdBuffer, barrierInfo.srcStageMask, barrierInfo.dstStageMask, 0,
        0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}

inline void BufferBarrier(
    VkCommandBuffer cmdBuffer,
    const std::vector<VkBuffer> &buffers,
    const BarrierInfo &barrierInfo)
{
    std::vector<VkBufferMemoryBarrier> bufBarriers(buffers.size());
    for (size_t i = 0; i != bufBarriers.size(); ++i) {
        bufBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufBarriers[i].srcQueueFamilyIndex = barrierInfo.srcQueueIdx;
        bufBarriers[i].dstQueueFamilyIndex = barrierInfo.dstQueueIdx;
        bufBarriers[i].offset = 0;
        bufBarriers[i].size = VK_WHOLE_SIZE;
        bufBarriers[i].srcAccessMask = barrierInfo.srcMask;
        bufBarriers[i].dstAccessMask = barrierInfo.dstMask;
        bufBarriers[i].buffer = buffers[i];
    }

    vkCmdPipelineBarrier(cmdBuffer, barrierInfo.srcStageMask, barrierInfo.dstStageMask, 0,
        0, nullptr, static_cast<uint32_t>(bufBarriers.size()), bufBarriers.data(), 0, nullptr);
}

inline void ImageBarrier(
    VkCommandBuffer cmdBuffer,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier &barrierInfo)
{
    vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
}
} // namespace Utils

#endif // IGU_UTILS_H
