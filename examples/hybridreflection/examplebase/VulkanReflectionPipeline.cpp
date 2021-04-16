/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan reflection pipeline implementation file
 */

#include "VulkanReflectionPipeline.h"
#include "VulkanPipelineCreateFuncMgr.h"
namespace vkpip {
REG_RENDER_PIPELINE(REFLECT_RENDER_PIPELINE, VulkanReflectionPipeline)
VulkanReflectionPipeline::~VulkanReflectionPipeline()
{
}
void VulkanReflectionPipeline::setupDescriptors()
{
    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3)};

    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      1),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      2),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      3),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      4)};
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
                                              &hitStorageBuffer->descriptor),
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3,
                                              &vertexBuffer->descriptor),
        vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4,
                                              &inderxBuffer->descriptor)};
    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
}

void VulkanReflectionPipeline::setupPipelines(const VkPipelineCache pipelineCache)
{
    this->setupInitialPipelineAttributes();
    // reflection pipeline
    pipelineCreateAttributes.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline))
}

void VulkanReflectionPipeline::draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene)
{
    // We must call preparePipelines first
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    uint32_t bindSet = 0;
    // set 0: 2 ubo buffers: matrices and params
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindSet, 1, &descriptorSet,
                            0, nullptr);
    // set 1: 3 ibl cube textures
    ibl->bindDescriptorSet(commandBuffer, pipelineLayout, bindSet + 1);
    // set 2: 5 marerial textures
    // set 3: 1 ubo matrices
    scene->draw(commandBuffer,
                vkglTF::RenderFlags::BindImages | vkglTF::RenderFlags::BindUbo |
                    vkglTF::RenderFlags::PushMaterialConst | vkglTF::RenderFlags::RenderReflection,
                pipelineLayout, bindSet + 2, 0);
}
} // namespace vkpip