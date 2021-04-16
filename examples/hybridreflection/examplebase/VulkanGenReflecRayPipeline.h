/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan generate reflect ray pipeline head file
 */

#ifndef VULKANEXAMPLES_VULKANGENREFLECRAYPIPELINE_H
#define VULKANEXAMPLES_VULKANGENREFLECRAYPIPELINE_H
#include "VulkanScenePipeline.h"
namespace vkpip {
class VulkanGenReflecRayPipeline : public VulkanScenePipeline {
public:
    VulkanGenReflecRayPipeline(vks::VulkanDevice *device, const ExtraPipelineResources *resources,
                               const std::string vertShaderName = "hybridreflection/scene.vert.spv",
                               const std::string fragShaderName = "hybridreflection/generateRay.frag.spv")
        : VulkanScenePipeline(device, resources, vertShaderName, fragShaderName), rayBuffer(resources->rayBuffer)
    {
    }
    ~VulkanGenReflecRayPipeline() = default;
    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    virtual void setupDescriptors() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;

private:
    vks::Buffer *rayBuffer;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANGENREFLECRAYPIPELINE_H
