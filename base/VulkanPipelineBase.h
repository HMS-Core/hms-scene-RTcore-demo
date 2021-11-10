/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan pipeline base head file
 */

#ifndef VULKANEXAMPLES_VULKANPIPELINEBASE_H
#define VULKANEXAMPLES_VULKANPIPELINEBASE_H

#include <array>

#include "vulkan/vulkan.h"

#include "SaschaWillemsVulkan/VulkanDevice.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "SaschaWillemsVulkan/camera.hpp"
#include "VulkanImageBasedLighting.h"
#include "VulkanShader.h"
#include "BufferInfor.h"
#include <memory>

namespace vkpip {
enum RenderPipelineType {
    SKY_BOX_PIPELINE,
    SCENE_PBR_PIPELINE,
    REFLECTION_BLEND_PIPELINE,
    DEPTH_ONLY_PIPELINE,
    GENERATE_RAY_PIPELINE,
    REFLECT_RENDER_PIPELINE,
    GBUFFER_RENDER_PIPELINE,
    DEFERRED_PBR_PIPELINE
};

struct ExtraPipelineResources {
    // For skybox render pipeline
    vks::TextureCubeMap *textureCubeMap = nullptr;
    // For on screen blend pipeline
    const VkDescriptorImageInfo *textureDescriptor = nullptr;
    // for generate ray pipeline
    vks::Buffer *rayBuffer = nullptr;
    // resources for scene and reflection pipeline
    vkibl::VulkanImageBasedLighting *ibl = nullptr;
    // resources for reflection pipeline
    vks::Buffer *hitBuffer = nullptr;
    vks::Buffer *vertexBuffer = nullptr;
    vks::Buffer *indexBuffer = nullptr;
};

struct PipelineDrawInfor {
    uint32_t indexCount;
    uint32_t startIndex = 0;
    vks::Buffer *vertexBuffer = nullptr;
    vks::Buffer *indexBuffer = nullptr;
};

struct PipelineShaderCreateInfor {
    std::string vertShaderName = "";
    std::string fragShaderName = "";
    VkPipelineShaderStageCreateInfo vertShader = {};
    VkPipelineShaderStageCreateInfo fragShader = {};
    bool CheckParams();
};

class VulkanPipelineBase {
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    VulkanPipelineBase(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                       const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        : device(vulkanDevice), pipelineShaderCreateInfor(pipelineShaderCreateInfor){};

    virtual ~VulkanPipelineBase() noexcept;

    virtual void Draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene){};
    virtual void Draw(VkCommandBuffer commandBuffer, PipelineDrawInfor *pipelineDrawInfor){};

    virtual void PreparePipelines(const VkRenderPass renderPass, const VkPipelineCache pipelineCache,
                                  const std::vector<VkDescriptorSetLayout> *setLayouts,
                                  const std::vector<VkPushConstantRange> *pushConstantRanges,
                                  const uint32_t subpass);
    virtual void UpdateMatrices(const buf::UBOMatrices *uboMatrices){};
    virtual void UpdateParams(const buf::UBOParams *uboParams){};
    virtual void UpdateStorageBuffers(VkQueue transferQueue){};

protected:
    vks::VulkanDevice *device = nullptr;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    // information of pipeline creation
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    struct PipelineCreateAttribute {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
        VkPipelineRasterizationStateCreateInfo rasterizationState;
        VkPipelineColorBlendAttachmentState blendAttachmentState;
        VkPipelineColorBlendStateCreateInfo colorBlendState;
        VkPipelineDepthStencilStateCreateInfo depthStencilState;
        VkPipelineViewportStateCreateInfo viewportState;
        VkPipelineMultisampleStateCreateInfo multisampleState;
        std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState;
        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        std::vector<VkPipelineShaderStageCreateInfo> shaders;
    } pipelineCreateAttributes;
    PipelineShaderCreateInfor pipelineShaderCreateInfor;
    virtual void SetupDescriptors(){};
    virtual void SetupPipelinelayout(const std::vector<VkDescriptorSetLayout> *setLayouts,
                                     const std::vector<VkPushConstantRange> *pushConstantRanges);
    virtual void SetupUniformBuffers(){};
    virtual void SetupStorageBuffers(){};
    virtual void SetupTextures(){};
    virtual void SetupPipelines(const VkPipelineCache pipelineCache) = 0;
    virtual void SetPipelineCreateInfor(const VkRenderPass renderPass, const uint32_t subpass);

private:
    void SetupShaderCreateInfors();
};
} // namespace vkpip

using RenderPipelinePtr = std::unique_ptr<vkpip::VulkanPipelineBase>;
#endif // VULKANEXAMPLES_VULKANPIPELINEBASE_H
