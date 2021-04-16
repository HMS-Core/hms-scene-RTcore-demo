/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan vertex head file
 */

#ifndef VULKANEXAMPLES_VULKANVERTEX_H
#define VULKANEXAMPLES_VULKANVERTEX_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "vulkan/vulkan.h"
namespace vkvert {
/**
 * glTF default vertex layout with easy Vulkan mapping functions
 */
enum class VertexComponent { Position, Normal, UV, Color, Tangent, Joint0, Weight0 };

struct Vertex {
    glm::vec4 pos;
    glm::vec4 normal;
    glm::vec4 uv;
    glm::vec4 color;
    glm::vec4 joint0;
    glm::vec4 weight0;
    glm::vec4 tangent;
    static VkVertexInputBindingDescription vertexInputBindingDescription;
    static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
    static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);
    static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location,
                                                                       VertexComponent component);
    static std::vector<VkVertexInputAttributeDescription>
    inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components);
    /** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components
     */
    static VkPipelineVertexInputStateCreateInfo *
    getPipelineVertexInputState(const std::vector<VertexComponent> components);
};
} // namespace vkvert
#endif // VULKANEXAMPLES_VULKANVERTEX_H
