/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan onscreen pipeline head file
 */

#ifndef VULKANEXAMPLES_VULKANONSCREENPIPELINE_H
#define VULKANEXAMPLES_VULKANONSCREENPIPELINE_H
#include "SaschaWillemsVulkan/VulkanDevice.h"
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanTexture.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "VulkanShader.h"
#include "VulkanScenePipeline.h"

namespace vkpip {
class VulkanOnscreenPipeline : public VulkanScenePipeline {
public:
    VulkanOnscreenPipeline(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                           const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : VulkanScenePipeline(vulkanDevice, resources, pipelineShaderCreateInfor),
          textureDescriptor(resources->textureDescriptor)
    {
    }
    ~VulkanOnscreenPipeline() override
    {
        textureDescriptor = nullptr;
    }
    void Draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    void SetupDescriptors() override;
    void SetupPipelines(const VkPipelineCache pipelineCache) override;

private:
    const VkDescriptorImageInfo *textureDescriptor = nullptr;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANONSCREENPIPELINE_H
