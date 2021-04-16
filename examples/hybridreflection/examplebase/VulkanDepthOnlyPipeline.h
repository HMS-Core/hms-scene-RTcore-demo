/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan depth only pipeline head file
 */
#include "VulkanScenePipeline.h"
#ifndef VULKANEXAMPLES_VULKANDEPTHONLYPIPELINE_H
#define VULKANEXAMPLES_VULKANDEPTHONLYPIPELINE_H
namespace vkpip {
class VulkanDepthOnlyPipeline : public VulkanScenePipeline {
public:
    VulkanDepthOnlyPipeline(vks::VulkanDevice *device, const ExtraPipelineResources *resources,
                            const std::string vertShaderName = "hybridreflection/scene.vert.spv",
                            const std::string fragShaderName = "")
        : VulkanScenePipeline(device, resources, vertShaderName, fragShaderName)
    {
    }
    ~VulkanDepthOnlyPipeline() = default;

public:
    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    //    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANDEPTHONLYPIPELINE_H
