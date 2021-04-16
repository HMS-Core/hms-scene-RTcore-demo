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
    REFLECT_RENDER_PIPELINE
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

class VulkanPipelineBase {
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    VulkanPipelineBase(vks::VulkanDevice *vulkanDevice, const ExtraPipelineResources *resources,
                       const std::string vertShaderName, const std::string fragShaderName = "")
        : device(vulkanDevice), vertShaderName(vertShaderName), fragShaderName(fragShaderName){};
    virtual ~VulkanPipelineBase();

    virtual void draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene){};
    virtual void draw(VkCommandBuffer commandBuffer, PipelineDrawInfor *pipelineDrawInfor){};

    virtual void preparePipelines(const VkRenderPass renderPass, const VkPipelineCache pipelineCache,
                                  const std::vector<VkDescriptorSetLayout> *setLayouts = nullptr,
                                  const std::vector<VkPushConstantRange> *pushConstantRanges = nullptr);
    virtual void updateMatrices(const buf::UBOMatrices *uboMatrices){};
    virtual void updateParams(const buf::UBOParams *uboParams){};
    virtual void updateStorageBuffers(VkQueue transferQueue = VK_NULL_HANDLE){};

protected:
    vks::VulkanDevice *device = nullptr;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    // information of pipeline creation
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
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

    std::string vertShaderName;
    std::string fragShaderName;

    virtual void setupDescriptors() = 0;
    virtual void setupPipelinelayout(const std::vector<VkDescriptorSetLayout> *setLayouts = nullptr,
                                     const std::vector<VkPushConstantRange> *pushConstantRanges = nullptr);
    virtual void setupUniformBuffers(){};
    virtual void setupStorageBuffers(){};
    virtual void setupTextures(){};
    virtual void setupPipelines(const VkPipelineCache pipelineCache){};
    virtual void setPipelineCreateInfor(const VkRenderPass renderPass);
};
} // namespace vkpip

using RenderPipelinePtr = std::unique_ptr<vkpip::VulkanPipelineBase>;
#endif // VULKANEXAMPLES_VULKANPIPELINEBASE_H
