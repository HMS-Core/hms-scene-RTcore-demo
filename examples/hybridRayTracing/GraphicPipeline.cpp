/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Vulkan GraphicPipeline implementation.
 */

#include "GraphicPipeline.h"

#include "Log.h"

namespace rt {
GraphicPipeline::GraphicPipeline(vks::VulkanDevice *vulkanDevice)
    : Pipeline(vulkanDevice),
      m_inputAssemblyState(detail::CreateInputAssemblyStateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE)),
      m_rasterizationState(detail::CreateRasterizationStateInfo(
          VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)),
      m_colorBlendState(detail::CreateColorBlendStateCreateInfo({})),
      m_depthStencilState(detail::CreateDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)),
      m_viewportState(detail::CreateViewportStateCreateInfo(1, 1)),
      m_multisampleState(detail::CreateMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT)),
      m_dynamicStates(std::vector<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}),
      m_dynamicState(detail::CreateDynamicStateCreateInfo(m_dynamicStates)),
      m_vertexInputState(detail::CreateVertexInputStateCreateInfo())
{
}

VkGraphicsPipelineCreateInfo GraphicPipeline::GetPipelineCreateInfo() const
{
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = m_pipelineLayout;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &m_rasterizationState;
    pipelineCreateInfo.pColorBlendState = &m_colorBlendState;
    pipelineCreateInfo.pMultisampleState = &m_multisampleState;
    pipelineCreateInfo.pViewportState = &m_viewportState;
    pipelineCreateInfo.pDepthStencilState = &m_depthStencilState;
    pipelineCreateInfo.pDynamicState = &m_dynamicState;
    pipelineCreateInfo.pVertexInputState = &m_vertexInputState;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    return pipelineCreateInfo;
}

bool GraphicPipeline::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout> &setLayouts,
                                           const std::vector<VkPushConstantRange> &pushConstantRanges)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

    VkResult res = vkCreatePipelineLayout(
        m_vulkanDevice->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    if (res != VK_SUCCESS) {
        m_pipelineLayout = VK_NULL_HANDLE;
        LOGE("%s: Failed to create PipelineLayout, err: %d", __func__, static_cast<int>(res));
        return false;
    }

    return true;
}

bool GraphicPipeline::Setup(const std::vector<VkDescriptorSetLayout> &setLayouts,
                            const std::vector<VkPushConstantRange> &pushConstantRanges,
                            VkRenderPass renderPass,
                            const std::vector<VkPipelineShaderStageCreateInfo> &shaderStages,
                            VkPipelineCache pipelineCache)
{
    if (m_vulkanDevice == nullptr || m_vulkanDevice->logicalDevice == VK_NULL_HANDLE) {
        return false;
    }

    if (!CreatePipelineLayout(setLayouts, pushConstantRanges)) {
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = GetPipelineCreateInfo();
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    VkResult res = vkCreateGraphicsPipelines(
        m_vulkanDevice->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    if (res != VK_SUCCESS) {
        LOGE("%s: Failed to create GraphicsPipeline, err: %d", __func__, static_cast<int>(res));
        return false;
    }

    return true;
}
} // namespace rt
