/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Vulkan GraphicPipeline declaration.
 */

#ifndef VULKANEXAMPLES_GRAPHICPIPELINE_H
#define VULKANEXAMPLES_GRAPHICPIPELINE_H

#include "Pipeline.h"

namespace rt {
namespace detail {
inline VkPipelineInputAssemblyStateCreateInfo CreateInputAssemblyStateInfo(
    VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable)
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo{};
    inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.topology = topology;
    inputAssemblyStateInfo.flags = flags;
    inputAssemblyStateInfo.primitiveRestartEnable = primitiveRestartEnable;
    return inputAssemblyStateInfo;
}

inline VkPipelineRasterizationStateCreateInfo CreateRasterizationStateInfo(
    VkPolygonMode polygonMode,
    VkCullModeFlags cullMode,
    VkFrontFace frontFace,
    VkPipelineRasterizationStateCreateFlags flags = 0)
{
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
    rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.polygonMode = polygonMode;
    rasterizationStateInfo.cullMode = cullMode;
    rasterizationStateInfo.frontFace = frontFace;
    rasterizationStateInfo.flags = flags;
    rasterizationStateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateInfo.lineWidth = 1.0f;
    return rasterizationStateInfo;
}

inline VkPipelineColorBlendAttachmentState CreateColorBlendAttachmentState(VkColorComponentFlags colorWriteMask,
                                                                           VkBool32 blendEnable)
{
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask = colorWriteMask;
    colorBlendAttachmentState.blendEnable = blendEnable;
    return colorBlendAttachmentState;
}

inline VkPipelineColorBlendStateCreateInfo CreateColorBlendStateCreateInfo(
    const std::vector<VkPipelineColorBlendAttachmentState> &attachments)
{
    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    colorBlendStateCreateInfo.pAttachments = attachments.data();
    return colorBlendStateCreateInfo;
}

inline VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStateCreateInfo(VkBool32 depthTestEnable,
                                                                               VkBool32 depthWriteEnable,
                                                                               VkCompareOp depthCompareOp)
{
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
    depthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
    depthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
    depthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
    return depthStencilStateCreateInfo;
}

inline VkPipelineViewportStateCreateInfo CreateViewportStateCreateInfo(uint32_t viewportCount,
                                                                       uint32_t scissorCount,
                                                                       VkPipelineViewportStateCreateFlags flags = 0)
{
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = viewportCount;
    viewportStateCreateInfo.scissorCount = scissorCount;
    viewportStateCreateInfo.flags = flags;
    return viewportStateCreateInfo;
}

inline VkPipelineMultisampleStateCreateInfo CreateMultisampleStateCreateInfo(
    VkSampleCountFlagBits rasterizationSamples, VkPipelineMultisampleStateCreateFlags flags = 0)
{
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
    multisampleStateCreateInfo.flags = flags;
    return multisampleStateCreateInfo;
}

inline VkPipelineDynamicStateCreateInfo CreateDynamicStateCreateInfo(const std::vector<VkDynamicState> &dynamicStates,
                                                                     VkPipelineDynamicStateCreateFlags flags = 0)
{
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.flags = flags;
    return dynamicStateCreateInfo;
}

inline VkPipelineVertexInputStateCreateInfo CreateVertexInputStateCreateInfo()
{
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    return vertexInputStateCreateInfo;
}
} // namespace detail

class GraphicPipeline : public Pipeline {
public:
    explicit GraphicPipeline(vks::VulkanDevice *vulkanDevice);
    ~GraphicPipeline() noexcept = default;

    void SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo &state)
    {
        m_inputAssemblyState = state;
    }

    void SetRasterizationState(const VkPipelineRasterizationStateCreateInfo &state)
    {
        m_rasterizationState = state;
    }

    void SetBlendAttachments(const std::vector<VkPipelineColorBlendAttachmentState> &attachments)
    {
        m_blendAttachmentStates = attachments;
        UpdateColorBlendState();
    }

    void SetBlendAttachments(std::vector<VkPipelineColorBlendAttachmentState> &&attachments)
    {
        m_blendAttachmentStates = std::move(attachments);
        UpdateColorBlendState();
    }

    void SetBlendState(const VkPipelineColorBlendStateCreateInfo &state)
    {
        m_colorBlendState = state;
    }

    void SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo &state)
    {
        m_depthStencilState = state;
    }

    void SetViewPortState(const VkPipelineViewportStateCreateInfo &state)
    {
        m_viewportState = state;
    }

    void SetMultisampleState(const VkPipelineMultisampleStateCreateInfo &state)
    {
        m_multisampleState = state;
    }

    void SetDynamicStates(const std::vector<VkDynamicState> &states)
    {
        m_dynamicStates = states;
    }

    void SetDynamicStates(std::vector<VkDynamicState> &&states)
    {
        m_dynamicStates = std::move(states);
    }

    void SetVertexInputBindings(const std::vector<VkVertexInputBindingDescription> &vertexInputBindings)
    {
        m_vertexInputBindings = vertexInputBindings;
        UpdateVertexInputBindingsState();
    }

    void SetVertexInputBindings(std::vector<VkVertexInputBindingDescription> &&vertexInputBindings)
    {
        m_vertexInputBindings = std::move(vertexInputBindings);
        UpdateVertexInputBindingsState();
    }

    void SetVertexInputAttributes(const std::vector<VkVertexInputAttributeDescription> &vertexInputAttributes)
    {
        m_vertexInputAttributes = vertexInputAttributes;
        UpdateVertexInputAttributesState();
    }

    void SetVertexInputAttributes(std::vector<VkVertexInputAttributeDescription> &&vertexInputAttributes)
    {
        m_vertexInputAttributes = std::move(vertexInputAttributes);
        UpdateVertexInputAttributesState();
    }

    bool Setup(const std::vector<VkDescriptorSetLayout> &setLayouts,
               const std::vector<VkPushConstantRange> &pushConstantRanges,
               VkRenderPass renderPass,
               const std::vector<VkPipelineShaderStageCreateInfo> &shaderStages,
               VkPipelineCache pipelineCache = VK_NULL_HANDLE);

private:
	void UpdateColorBlendState()
    {
        m_colorBlendState.attachmentCount = static_cast<uint32_t>(m_blendAttachmentStates.size());
        m_colorBlendState.pAttachments = m_blendAttachmentStates.data();
    }

    void UpdateVertexInputAttributesState()
    {
        m_vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributes.size());
        m_vertexInputState.pVertexAttributeDescriptions = m_vertexInputAttributes.data();
    }

    void UpdateVertexInputBindingsState()
    {
        m_vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindings.size());
        m_vertexInputState.pVertexBindingDescriptions = m_vertexInputBindings.data();
    }

    bool CreatePipelineLayout(const std::vector<VkDescriptorSetLayout> &setLayouts,
                              const std::vector<VkPushConstantRange> &pushConstantRanges);
    VkGraphicsPipelineCreateInfo GetPipelineCreateInfo() const;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyState;
    VkPipelineRasterizationStateCreateInfo m_rasterizationState;
    std::vector<VkPipelineColorBlendAttachmentState> m_blendAttachmentStates;
    VkPipelineColorBlendStateCreateInfo m_colorBlendState;
    VkPipelineDepthStencilStateCreateInfo m_depthStencilState;
    VkPipelineViewportStateCreateInfo m_viewportState;
    VkPipelineMultisampleStateCreateInfo m_multisampleState;
    std::vector<VkDynamicState> m_dynamicStates;
    VkPipelineDynamicStateCreateInfo m_dynamicState;

    std::vector<VkVertexInputBindingDescription> m_vertexInputBindings;
    std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributes;
    VkPipelineVertexInputStateCreateInfo m_vertexInputState;
};
} // namespace rt
#endif // VULKANEXAMPLES_GRAPHICPIPELINE_H
