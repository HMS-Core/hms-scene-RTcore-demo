/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan GenBRDFFlut RenderCube pipelines' header
 */

#ifndef VULKANEXAMPLES_VULKANIBLPIPELINES_H
#define VULKANEXAMPLES_VULKANIBLPIPELINES_H
#include "VulkanPipelineBase.h"

namespace vkpip {
class VulkanGenBRDFFlutPipeline : public VulkanPipelineBase {
public:
    VulkanGenBRDFFlutPipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                              const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : VulkanPipelineBase(vulkanDevice, resources, pipelineShaderCreateInfor)
    {
    }
    ~VulkanGenBRDFFlutPipeline() = default;

protected:
    void SetupDescriptors() override;
    void SetupPipelines(const VkPipelineCache pipelineCache) override;
};

class VulkanRenderCubePipeline : public VulkanPipelineBase {
public:
    VulkanRenderCubePipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                             const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : VulkanPipelineBase(vulkanDevice, resources, pipelineShaderCreateInfor),
          textureCubeMap(resources->textureCubeMap)
    {
    }
    ~VulkanRenderCubePipeline() = default;

protected:
    vks::TextureCubeMap *textureCubeMap = nullptr;
    void SetupDescriptors() override;
    void SetupPipelines(const VkPipelineCache pipelineCache) override;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANIBLPIPELINES_H
