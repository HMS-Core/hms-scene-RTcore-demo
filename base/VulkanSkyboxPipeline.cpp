/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan skybox implementation file
 */

#include "VulkanSkyboxPipeline.h"
#include "VulkanPipelineCreateFuncMgr.h"

namespace vkpip {
VulkanSkyboxPipeline::~VulkanSkyboxPipeline() noexcept
{
    uniformBuffers.matrices.destroy();
    uniformBuffers.params.destroy();
    environmentCube->destroy();
}


void VulkanSkyboxPipeline::SetupDescriptors()
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
                                                      VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };
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
                                              &environmentCube->descriptor),
    };
    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
}

void VulkanSkyboxPipeline::SetupPipelines(const VkPipelineCache pipelineCache)
{
    pipelineCreateAttributes.vertexInputState = *vkvert::Vertex::GetPipelineVertexInputState(
        {vkvert::VertexComponent::Position, vkvert::VertexComponent::Normal,
        vkvert::VertexComponent::UV});
    // Skybox pipeline (background cube)
    pipelineCreateAttributes.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineCreateAttributes.depthStencilState.depthTestEnable = VK_TRUE;
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void VulkanSkyboxPipeline::SetupUniformBuffers()
{
    // Skybox vertex shader uniform buffer
    VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         &uniformBuffers.matrices, sizeof(buf::UBOMatrices)));

    // Shared parameter uniform buffer
    VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         &uniformBuffers.params, sizeof(buf::UBOParams)));

    // Map persistent
    VK_CHECK_RESULT(uniformBuffers.matrices.map());
    VK_CHECK_RESULT(uniformBuffers.params.map());
}

void VulkanSkyboxPipeline::Draw(VkCommandBuffer commandBuffer, vkglTF::Model *skybox)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    skybox->draw(commandBuffer);
}

void VulkanSkyboxPipeline::UpdateMatrices(const buf::UBOMatrices *uboMatrices)
{
    memcpy(uniformBuffers.matrices.mapped, uboMatrices, sizeof(buf::UBOMatrices));
}

void VulkanSkyboxPipeline::UpdateParams(const buf::UBOParams *params)
{
    memcpy(uniformBuffers.params.mapped, params, sizeof(buf::UBOParams));
}
} // namespace vkpip