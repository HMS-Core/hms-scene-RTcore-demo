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
#include "Log.h"
#include "Utils.h"
namespace rt {
struct BVHVertexBuffers {
    vks::Buffer vertex;
    vks::Buffer index;
    vks::Buffer stagingBuffer;
};

struct RayTracingDescriptors {
    VkDescriptorBufferInfo bvhTree = {};
    VkDescriptorBufferInfo bvhTriangles = {};
    VkDescriptorBufferInfo tlasInfo = {};
    VkDescriptorBufferInfo rtCoreUniforms = {};
};

class VulkanTraceRay {
public:
    VulkanTraceRay(vks::VulkanDevice *vulkanDevice, uint32_t traceWidth, uint32_t traceHeight, vks::Buffer *hitBuffer,
                   vks::Buffer *rayBuffer)
        : device(vulkanDevice),
          height(traceHeight),
          width(traceWidth),
          rayCount(height * width),
          hitBuffer(hitBuffer),
          rayBuffer(rayBuffer){};
    explicit VulkanTraceRay(vks::VulkanDevice *vulkanDevice) : device(vulkanDevice){};
    ~VulkanTraceRay() noexcept;

    void Prepare();
    void BuildBVH(std::vector<vkvert::Vertex> *vertices, std::vector<uint32_t> *indices, const glm::mat4 &modelMatrix);
    void BuildBVH(vkglTF::Model *scene, const glm::mat4 &modelMatrix);
    void TraceRay(VkCommandBuffer cmd = VK_NULL_HANDLE);
    void UpdateBVH(const glm::mat4 &modelMatrix);
    void RefitBVH(VkCommandBuffer cmd);
    RayTracingDescriptors GetRaytracingDescriptors();
    void DestroyBVH();
    void CreateRaytraceShaders(std::string shaderPath, std::vector<std::string> includeShadersKey = {},
                               std::vector<std::string> includeShadersPath = {});
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};

private:
    vks::VulkanDevice *device = nullptr;
    vkglTF::Model *gltfScene = nullptr;
    vks::Buffer *hitBuffer = nullptr;
    vks::Buffer *rayBuffer = nullptr;
    uint32_t height = 0;
    uint32_t width = 0;
    uint32_t rayCount = 0;
    VkQueue computeQueue = VK_NULL_HANDLE;

    // Resources for RT Core
    std::unique_ptr<RayShop::Vulkan::Traversal> traversal;
    std::vector<vkvert::Vertex> worldVertices;
    std::vector<RayShop::GeometryTriangleDescription> bvhGeometriesOnCPU;
    std::vector<RayShop::GeometryTriangleDescription> bvhGeometriesOnGPU;
    std::vector<RayShop::BLAS> blases;
    BVHVertexBuffers bvhBuffers;
    bool prepareFlag = false;
    bool bvhBuildFlag = false;

    void ConvertLocalToWorld(std::vector<vkvert::Vertex> *vertices, const glm::mat4 &modelMatrix);
    void BuildBVH(std::vector<uint32_t> *indices);
    RayTracingDescriptors rayTracingDescriptors;
    std::string entryPoint = "main";
    std::unique_ptr<Utils::ScopedShaderModule> shaderModule;
};
} // namespace rt

#endif // VULKANEXAMPLES_VULKANTRACERAY_H
