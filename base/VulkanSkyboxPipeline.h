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
                         const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : VulkanPipelineBase(vulkanDevice, resources, pipelineShaderCreateInfor),
          environmentCube(resources->textureCubeMap){};
    ~VulkanSkyboxPipeline() noexcept override;
    void Draw(VkCommandBuffer commandBuffer, vkglTF::Model *skybox) override;
    void UpdateMatrices(const buf::UBOMatrices *uboMatrices) override;
    void UpdateParams(const buf::UBOParams *params) override;

protected:
    void SetupDescriptors() override;
    void SetupPipelines(const VkPipelineCache pipelineCache) override;
    // Prepare and initialize uniform buffer containing shader uniforms
    void SetupUniformBuffers() override;

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
