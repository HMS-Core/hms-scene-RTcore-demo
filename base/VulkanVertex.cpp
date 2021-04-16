/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan vertex implementation file
 */

#include "VulkanVertex.h"
namespace vkvert {
/*
    glTF default vertex layout with easy Vulkan mapping functions
*/
VkVertexInputBindingDescription Vertex::vertexInputBindingDescription;
std::vector<VkVertexInputAttributeDescription> Vertex::vertexInputAttributeDescriptions;
VkPipelineVertexInputStateCreateInfo Vertex::pipelineVertexInputStateCreateInfo;

VkVertexInputBindingDescription Vertex::inputBindingDescription(uint32_t binding)
{
    return VkVertexInputBindingDescription({binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
}

VkVertexInputAttributeDescription Vertex::inputAttributeDescription(uint32_t binding, uint32_t location,
                                                                    VertexComponent component)
{
    switch (component) {
        case VertexComponent::Position:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, pos)});
        case VertexComponent::Normal:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        case VertexComponent::UV:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
        case VertexComponent::Color:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)});
        case VertexComponent::Tangent:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)});
        case VertexComponent::Joint0:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, joint0)});
        case VertexComponent::Weight0:
            return VkVertexInputAttributeDescription({
                location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weight0)});
        default:
            return VkVertexInputAttributeDescription({});
    }
}

std::vector<VkVertexInputAttributeDescription> Vertex::inputAttributeDescriptions(uint32_t binding,
    const std::vector<VertexComponent> components)
{
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t location = 0;
    for (VertexComponent component : components) {
        result.push_back(Vertex::inputAttributeDescription(binding, location, component));
        location++;
    }
    return result;
}

/** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
VkPipelineVertexInputStateCreateInfo *Vertex::getPipelineVertexInputState(const std::vector<VertexComponent> components)
{
    vertexInputBindingDescription = Vertex::inputBindingDescription(0);
    Vertex::vertexInputAttributeDescriptions = Vertex::inputAttributeDescriptions(0, components);
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &Vertex::vertexInputBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(Vertex::vertexInputAttributeDescriptions.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions.data();
    return &pipelineVertexInputStateCreateInfo;
}
} // namespace vkvert