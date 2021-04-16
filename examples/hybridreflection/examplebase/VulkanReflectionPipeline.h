/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan reflection pipeline head file
 */

#ifndef VULKANEXAMPLES_VULKANPOSTPROCESSREFLECTION_H
#define VULKANEXAMPLES_VULKANPOSTPROCESSREFLECTION_H
#include "VulkanScenePipeline.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "VulkanImageBasedLighting.h"
#include "SaschaWillemsVulkan/VulkanTools.h"
#include "Traversal.h"

namespace vkpip {
class VulkanReflectionPipeline : public VulkanScenePipeline {
public:
    VulkanReflectionPipeline(vks::VulkanDevice *device, const ExtraPipelineResources *resources,
                             const std::string vertShaderName = "hybridreflection/scene.vert.spv",
                             const std::string fragShaderName = "hybridreflection/reflection.frag.spv")
        : VulkanScenePipeline(device, resources, vertShaderName, fragShaderName),
          hitStorageBuffer(resources->hitBuffer),
          vertexBuffer(resources->vertexBuffer),
          inderxBuffer(resources->indexBuffer){};
    virtual ~VulkanReflectionPipeline();

    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    virtual void setupDescriptors() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;

private:
    vks::Buffer *hitStorageBuffer = nullptr;
    vks::Buffer *vertexBuffer = nullptr;
    vks::Buffer *inderxBuffer = nullptr;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANPOSTPROCESSREFLECTION_H
