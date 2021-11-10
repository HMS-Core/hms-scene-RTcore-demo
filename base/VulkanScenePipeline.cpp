/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan scene pipeline implementation file
 */

#include "VulkanScenePipeline.h"
#include "VulkanPipelineCreateFuncMgr.h"

namespace vkpip {
VulkanScenePipeline::~VulkanScenePipeline() noexcept
{
    uniformBuffers.matrices.destroy();
    uniformBuffers.params.destroy();
    ibl = nullptr;
}

void VulkanScenePipeline::SetupInitialPipelineAttributes()
{
    pipelineCreateAttributes.vertexInputState = *vkvert::Vertex::GetPipelineVertexInputState(
        {vkvert::VertexComponent::Position, vkvert::VertexComponent::Normal,
            vkvert::VertexComponent::UV, vkvert::VertexComponent::Joint0,
            vkvert::VertexComponent::Weight0});
    pipelineCreateAttributes.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

    // Enable depth test and write
    pipelineCreateAttributes.depthStencilState.depthWriteEnable = VK_TRUE;
    pipelineCreateAttributes.depthStencilState.depthTestEnable = VK_TRUE;
}

void VulkanScenePipeline::SetupPipelines(const VkPipelineCache pipelineCache)
{
    this->SetupInitialPipelineAttributes();
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void VulkanScenePipeline::SetupUniformBuffers()
{
    uniformBuffers.matrices.destroy();
    uniformBuffers.params.destroy();

    // pbr vertex shader uniform buffer
    VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         &uniformBuffers.matrices, sizeof(buf::UBOMatrices)));

    // Shared parameter uniform buffer
    VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         &uniformBuffers.params, sizeof(buf::UBOParams)));

    // Map persistent
    VK_CHECK_RESULT(uniformBuffers.params.map());
    VK_CHECK_RESULT(uniformBuffers.matrices.map());
}

void VulkanScenePipeline::SetupDescriptors()
{
    // Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)};

    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      1)};
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
                                              &uniformBuffers.params.descriptor)};
    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, NULL);
}

void VulkanScenePipeline::UpdateMatrices(const buf::UBOMatrices *uboMatrices)
{
    memcpy(uniformBuffers.matrices.mapped, uboMatrices, sizeof(buf::UBOMatrices));
}

void VulkanScenePipeline::UpdateParams(const buf::UBOParams *params)
{
    memcpy(uniformBuffers.params.mapped, params, sizeof(buf::UBOParams));
}

void VulkanScenePipeline::Draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene)
{
    // We must call PreparePipelines first
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    uint32_t bindSet = 0;
    // set 0: 2 ubo buffers: matrices and params
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindSet, 1, &descriptorSet,
                            0, nullptr);
    // set 1: 3 ibl cube textures
    ibl->BindDescriptorSet(commandBuffer, pipelineLayout, bindSet + 1);
    // set 2: 5 marerial textures
    // set 3: 1 ubo matrices
    scene->draw(commandBuffer,
                vkglTF::RenderFlags::BindImages | vkglTF::RenderFlags::BindUbo | vkglTF::RenderFlags::PushMaterialConst,
                pipelineLayout, bindSet + 2);
}
} // namespace vkpip