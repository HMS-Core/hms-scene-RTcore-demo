/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan pipeline base implementation file
 */

#include "VulkanPipelineBase.h"
namespace vkpip {
VulkanPipelineBase::~VulkanPipelineBase()
{
    vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
}

void VulkanPipelineBase::preparePipelines(const VkRenderPass renderPass, const VkPipelineCache pipelineCache,
                                          const std::vector<VkDescriptorSetLayout> *setLayouts,
                                          const std::vector<VkPushConstantRange> *pushConstantRanges)
{
    setupUniformBuffers();
    setupStorageBuffers();
    setupDescriptors();
    setupTextures();
    setupPipelinelayout(setLayouts, pushConstantRanges);
    setPipelineCreateInfor(renderPass);
    setupPipelines(pipelineCache);
}

void VulkanPipelineBase::setupPipelinelayout(const std::vector<VkDescriptorSetLayout> *setLayouts,
                                             const std::vector<VkPushConstantRange> *pushConstantRanges)
{
    std::vector<VkDescriptorSetLayout> pipelineLayouts;
    if (setLayouts) {
        pipelineLayouts = *setLayouts;
    } else {
        pipelineLayouts = {};
    }
    pipelineLayouts.insert(pipelineLayouts.begin(), descriptorSetLayout);
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(
        pipelineLayouts.data(), static_cast<uint32_t>(pipelineLayouts.size()));
    if (pushConstantRanges) {
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges->size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges->data();
    }

    VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void VulkanPipelineBase::setPipelineCreateInfor(const VkRenderPass renderPass)
{
    pipelineCreateAttributes.inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    pipelineCreateAttributes.rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    // Blend attachment states required for all color attachments
    // This is important, as color write mask will otherwise be 0x0 and you
    // won't see anything rendered to the attachment
    pipelineCreateAttributes.blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    pipelineCreateAttributes.colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(1.0, &pipelineCreateAttributes.blendAttachmentState);
    pipelineCreateAttributes.depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineCreateAttributes.viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    pipelineCreateAttributes.multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    pipelineCreateAttributes.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipelineCreateAttributes.dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(pipelineCreateAttributes.dynamicStateEnables);
    pipelineCreateAttributes.shaders.clear();
    pipelineCreateAttributes.shaders.push_back(
        vks::LoadShaders::loadShader(device, vertShaderName.c_str(), VK_SHADER_STAGE_VERTEX_BIT));
    if (!fragShaderName.empty()) {
        pipelineCreateAttributes.shaders.push_back(
            vks::LoadShaders::loadShader(device, fragShaderName.c_str(), VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    // We must call setupPipelinelayout first
    pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
    pipelineCreateInfo.pInputAssemblyState = &pipelineCreateAttributes.inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &pipelineCreateAttributes.rasterizationState;
    pipelineCreateInfo.pColorBlendState = &pipelineCreateAttributes.colorBlendState;
    pipelineCreateInfo.pMultisampleState = &pipelineCreateAttributes.multisampleState;
    pipelineCreateInfo.pViewportState = &pipelineCreateAttributes.viewportState;
    pipelineCreateInfo.pDepthStencilState = &pipelineCreateAttributes.depthStencilState;
    pipelineCreateInfo.pDynamicState = &pipelineCreateAttributes.dynamicState;
    pipelineCreateInfo.stageCount = pipelineCreateAttributes.shaders.size();
    pipelineCreateInfo.pStages = pipelineCreateAttributes.shaders.data();

    // Default empty vertex input
    pipelineCreateAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineCreateAttributes.vertexInputState.vertexAttributeDescriptionCount = 0;
    pipelineCreateAttributes.vertexInputState.pVertexAttributeDescriptions = nullptr;
    pipelineCreateAttributes.vertexInputState.vertexBindingDescriptionCount = 0;
    pipelineCreateAttributes.vertexInputState.pVertexBindingDescriptions = nullptr;

    pipelineCreateInfo.pVertexInputState = &pipelineCreateAttributes.vertexInputState;
}
} // namespace vkpip