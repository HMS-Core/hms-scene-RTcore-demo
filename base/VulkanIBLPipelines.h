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
                              const std::string vertShaderName = "hybridreflection/genbrdflut.vert.spv",
                              const std::string fragShaderName = "hybridreflection/genbrdflut.frag.spv")
        : VulkanPipelineBase(vulkanDevice, resources, vertShaderName, fragShaderName)
    {
    }
    ~VulkanGenBRDFFlutPipeline() = default;

protected:
    virtual void setupDescriptors() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;
};

class VulkanRenderCubePipeline : public VulkanPipelineBase {
public:
    VulkanRenderCubePipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                             const std::string vertShaderName = "hybridreflection/filtercube.vert.spv",
                             const std::string fragShaderName = "hybridreflection/irradiancecube.frag.spv")
        : VulkanPipelineBase(vulkanDevice, resources, vertShaderName, fragShaderName),
          textureCubeMap(resources->textureCubeMap)
    {
    }
    ~VulkanRenderCubePipeline() = default;

protected:
    vks::TextureCubeMap *textureCubeMap = nullptr;
    virtual void setupDescriptors() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANIBLPIPELINES_H
