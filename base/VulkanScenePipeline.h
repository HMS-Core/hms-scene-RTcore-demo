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
                        const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : VulkanPipelineBase(vulkanDevice, resources, pipelineShaderCreateInfor), ibl(resources->ibl){};

    ~VulkanScenePipeline() noexcept override;
    void UpdateMatrices(const buf::UBOMatrices *uboMatrices) override;
    void UpdateParams(const buf::UBOParams *uboParams) override;
    void Draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene) override;

protected:
    vkibl::VulkanImageBasedLighting *ibl = nullptr;
    struct UniformBufferSet {
        vks::Buffer matrices;
        vks::Buffer params;
    } uniformBuffers;

protected:
    void SetupDescriptors() override;
    void SetupUniformBuffers() override;
    void SetupPipelines(const VkPipelineCache pipelineCache) override;
    virtual void SetupInitialPipelineAttributes();
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANSCENEPIPELINE_H
