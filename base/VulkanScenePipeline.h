/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan scene pipeline head file
 */

#ifndef VULKANEXAMPLES_VULKANSCENEPIPELINE_H
#define VULKANEXAMPLES_VULKANSCENEPIPELINE_H
#include <array>
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanInitializers.hpp"
#include "SaschaWillemsVulkan/VulkanDevice.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "BufferInfor.h"
#include "VulkanImageBasedLighting.h"
#include "VulkanPipelineBase.h"
namespace vkpip {
class VulkanScenePipeline : public VulkanPipelineBase {
public:
    VulkanScenePipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                        const std::string vertShaderName = "hybridreflection/scene.vert.spv",
                        const std::string fragShaderName = "hybridreflection/pbr.frag.spv")
        : VulkanPipelineBase(vulkanDevice, resources, vertShaderName, fragShaderName), ibl(resources->ibl){};

    virtual ~VulkanScenePipeline();
    virtual void updateMatrices(const buf::UBOMatrices *uboMatrices) override;
    virtual void updateParams(const buf::UBOParams *uboParams) override;

    virtual void preparePipelines(const VkRenderPass renderPass, const VkPipelineCache pipelineCache,
                                  const std::vector<VkDescriptorSetLayout> *setLayouts = nullptr,
                                  const std::vector<VkPushConstantRange> *pushConstantRanges = nullptr) override;
    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    vkibl::VulkanImageBasedLighting *ibl = nullptr;
    struct UniformBufferSet {
        vks::Buffer matrices;
        vks::Buffer params;
    } uniformBuffers;

protected:
    virtual void setupDescriptors() override;
    virtual void setupUniformBuffers() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;
    virtual void setupInitialPipelineAttributes();
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANSCENEPIPELINE_H
