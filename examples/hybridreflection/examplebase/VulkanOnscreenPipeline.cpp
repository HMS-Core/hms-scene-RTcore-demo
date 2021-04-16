/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan onscreen pipeline implementation file
 */

#include "VulkanOnscreenPipeline.h"
#include "VulkanPipelineCreateFuncMgr.h"

namespace vkpip {
REG_RENDER_PIPELINE(REFLECTION_BLEND_PIPELINE, VulkanOnscreenPipeline)
void VulkanOnscreenPipeline::setupDescriptors()
{
    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      1),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                      VK_SHADER_STAGE_FRAGMENT_BIT, 2)};

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

    // Descriptor sets
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet));

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                              &uniformBuffers.matrices.descriptor),
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                              &uniformBuffers.params.descriptor),
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                              textureDescriptor),
    };
    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
}

void VulkanOnscreenPipeline::setupPipelines(const VkPipelineCache pipelineCache)
{
    this->setupInitialPipelineAttributes();

    pipelineCreateAttributes.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

    pipelineCreateAttributes.blendAttachmentState.blendEnable = VK_TRUE;
    pipelineCreateAttributes.blendAttachmentState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
    pipelineCreateAttributes.blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
    pipelineCreateAttributes.blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    pipelineCreateAttributes.blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineCreateAttributes.colorBlendState.blendConstants[0] = BLEND_PARAM;
    pipelineCreateAttributes.colorBlendState.blendConstants[1] = BLEND_PARAM;
    pipelineCreateAttributes.colorBlendState.blendConstants[2] = BLEND_PARAM;
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void VulkanOnscreenPipeline::draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // set 0:  2 ubo buffers: matrices and params, 1 texture
    uint32_t bindSet = 0;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            nullptr);
    // set 2: 5 material textures not used here
    // set 3: 1 ubo matrices
    scene->draw(commandBuffer, vkglTF::RenderFlags::BindUbo, pipelineLayout, bindSet + 2, 0);
}
} // namespace vkpip
