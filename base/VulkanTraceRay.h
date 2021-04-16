/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan trace ray head file
 */

#ifndef VULKANEXAMPLES_VULKANTRACERAY_H
#define VULKANEXAMPLES_VULKANTRACERAY_H
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "Traversal.h"
namespace rt {
class VulkanTraceRay {
public:
    VulkanTraceRay(vks::VulkanDevice *vulkanDevice, uint32_t traceWidth, uint32_t traceHeight, vks::Buffer *hitBuffer)
        : device(vulkanDevice),
          height(traceHeight),
          width(traceWidth),
          rayCount(height * width),
          hitBuffer(hitBuffer){};
    ~VulkanTraceRay() = default;

    void prepare();
    void buildBVH(std::vector<vkvert::Vertex> *vertices, std::vector<uint32_t> *indices, const glm::mat4 &modelMatrix);
    void buildBVH(vkglTF::Model *scene, const glm::mat4 &modelMatrix);
    void traceRay(vks::Buffer *rayBuffer);
private:
    vks::VulkanDevice *device = nullptr;
    vks::Buffer *hitBuffer = nullptr;
    uint32_t height = 0;
    uint32_t width = 0;
    uint32_t rayCount = 0;
    VkQueue computeQueue = VK_NULL_HANDLE;

    // Resources for RT Core
    RayShop::Traversal traversal;
    std::vector<vkvert::Vertex> worldVertices;

private:
    void convertLocalToWorld(std::vector<vkvert::Vertex> *vertices, const glm::mat4 &modelMatrix);
    void buildBVH(std::vector<uint32_t> *indices);
};
} // namespace rt

#endif // VULKANEXAMPLES_VULKANTRACERAY_H
