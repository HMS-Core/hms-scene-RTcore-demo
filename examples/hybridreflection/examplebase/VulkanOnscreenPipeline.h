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
                           const std::string vertShaderName = "hybridreflection/scene.vert.spv",
                           const std::string fragShaderName = "hybridreflection/fullscreen.frag.spv")
        : VulkanScenePipeline(vulkanDevice, resources, vertShaderName, fragShaderName),
          textureDescriptor(resources->textureDescriptor)
    {
    }
    virtual ~VulkanOnscreenPipeline() = default;
    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    virtual void setupDescriptors() override;
    virtual void setupPipelines(const VkPipelineCache pipelineCache) override;

private:
    const VkDescriptorImageInfo *textureDescriptor = nullptr;

    static constexpr float BLEND_PARAM = 0.8;
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANONSCREENPIPELINE_H
