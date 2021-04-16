/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan generate reflect ray pipeline
 */

#include "VulkanGenReflecRayPipeline.h"
#include "VulkanPipelineCreateFuncMgr.h"
namespace vkpip {
REG_RENDER_PIPELINE(GENERATE_RAY_PIPELINE, VulkanGenReflecRayPipeline)
void VulkanGenReflecRayPipeline::setupDescriptors()
{
    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)};

    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      1),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      2)};
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

    // Descriptor sets
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet))

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                              &uniformBuffers.matrices.descriptor),
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                              &uniformBuffers.params.descriptor),
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2,
                                              &rayBuffer->descriptor)};
    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
}

void VulkanGenReflecRayPipeline::setupPipelines(const VkPipelineCache pipelineCache)
{
    this->setupInitialPipelineAttributes();
    // generate pass doesn't use any color attachments
    pipelineCreateAttributes.colorBlendState.attachmentCount = 0;
    pipelineCreateAttributes.colorBlendState.pAttachments = nullptr;
    pipelineCreateAttributes.depthStencilState.depthWriteEnable = VK_FALSE;
    pipelineCreateAttributes.depthStencilState.depthTestEnable = VK_TRUE;
    // reflection pipeline
    pipelineCreateAttributes.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline))
}

void VulkanGenReflecRayPipeline::draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // set 0:  2 ubo buffers: matrices and params, 1 texture
    uint32_t bindSet = 0;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            nullptr);
    // set 2: 5 material textures not used here
    // set 3: 1 ubo matrices
    scene->draw(commandBuffer,
                vkglTF::RenderFlags::BindUbo | vkglTF::RenderFlags::BindImages | vkglTF::RenderFlags::PushMaterialConst,
                pipelineLayout, bindSet + 2, 0);
}
} // namespace vkpip