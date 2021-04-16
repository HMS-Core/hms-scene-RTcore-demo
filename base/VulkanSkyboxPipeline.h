/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan skybox head file
 */

#ifndef VULKANEXAMPLES_VULKANSKYBOXPIPELINE_H
#define VULKANEXAMPLES_VULKANSKYBOXPIPELINE_H
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "SaschaWillemsVulkan/VulkanTexture.h"
#include "VulkanPipelineBase.h"
#include "VulkanShader.h"
#include "BufferInfor.h"

namespace vkpip {
class VulkanSkyboxPipeline : public VulkanPipelineBase {
public:
    VulkanSkyboxPipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                         const std::string vertShaderName = "hybridreflection/skybox.vert.spv",
                         const std::string fragShaderName = "hybridreflection/skybox.frag.spv")
        : VulkanPipelineBase(vulkanDevice, resources, vertShaderName, fragShaderName),
          environmentCube(resources->textureCubeMap){};
    virtual ~VulkanSkyboxPipeline();
    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *skybox) override;
    virtual void preparePipelines(const VkRenderPass renderPass, const VkPipelineCache pipelineCache,
                                  const std::vector<VkDescriptorSetLayout> *setLayouts = nullptr,
                                  const std::vector<VkPushConstantRange> *pushConstantRanges = nullptr) override;
    virtual void updateMatrices(const buf::UBOMatrices *uboMatrices) override;
    virtual void updateParams(const buf::UBOParams *params) override;

protected:
    virtual void setupDescriptors() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;
    // Prepare and initialize uniform buffer containing shader uniforms
    virtual void setupUniformBuffers() override;

private:
    vks::TextureCubeMap *environmentCube;

private:
    struct UniformBufferSet {
        vks::Buffer matrices;
        vks::Buffer params;
    } uniformBuffers;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANSKYBOXPIPELINE_H
