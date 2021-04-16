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
                           std::string vertShaderName = "triangle/fullscreen.vert.spv",
                           std::string fragShaderName = "triangle/showhit.frag.spv")
        : VulkanPipelineBase(vulkanDevice, resources, vertShaderName, fragShaderName),
          hitStorageBuffer(resources->hitBuffer),
          vertexBuffer(resources->vertexBuffer),
          indexBuffer(resources->indexBuffer){};
    virtual ~VulkanTrianglePipeline();
    virtual void updateParams(const buf::UBOParams *uboParams) override;
    virtual void draw(VkCommandBuffer commandBuffer, PipelineDrawInfor *pipelineDrawInfor) override;

protected:
    struct UniformBufferSet {
        vks::Buffer params;
    } uniformBuffers;

    vks::Buffer *vertexBuffer = nullptr;
    vks::Buffer *indexBuffer = nullptr;
    vks::Buffer *hitStorageBuffer = nullptr;

    virtual void setupDescriptors() override;
    virtual void setupUniformBuffers() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANTRIANGLEPIPELINE_H
