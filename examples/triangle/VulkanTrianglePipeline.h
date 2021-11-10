/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan show hit render pipeline head file
 */

#ifndef VULKANEXAMPLES_VULKANTRIANGLEPIPELINE_H
#define VULKANEXAMPLES_VULKANTRIANGLEPIPELINE_H

#include "VulkanPipelineBase.h"
namespace vkpip {
class VulkanTrianglePipeline : public VulkanPipelineBase {
public:
    VulkanTrianglePipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                           const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : VulkanPipelineBase(vulkanDevice, resources, pipelineShaderCreateInfor),
          hitStorageBuffer(resources->hitBuffer),
          vertexBuffer(resources->vertexBuffer),
          indexBuffer(resources->indexBuffer){};
    ~VulkanTrianglePipeline() noexcept override;
    void UpdateParams(const buf::UBOParams *uboParams) override;
    void Draw(VkCommandBuffer commandBuffer, PipelineDrawInfor *pipelineDrawInfor) override;

protected:
    struct UniformBufferSet {
        vks::Buffer params;
    } uniformBuffers;

    vks::Buffer *vertexBuffer = nullptr;
    vks::Buffer *indexBuffer = nullptr;
    vks::Buffer *hitStorageBuffer = nullptr;

    void SetupDescriptors() override;
    void SetupUniformBuffers() override;
    void SetupPipelines(const VkPipelineCache pipelineCache) override;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANTRIANGLEPIPELINE_H
