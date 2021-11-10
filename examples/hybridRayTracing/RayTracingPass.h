/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Vulkan RayTracing RenderPass declaration.
 */

#ifndef VULKANEXAMPLES_RAYTRACINGPASS_H
#define VULKANEXAMPLES_RAYTRACINGPASS_H

#include "NonCopyable.h"

#include "SaschaWillemsVulkan/VulkanFrameBuffer.hpp"
#include "BufferInfor.h"
#include "DescSet.h"
#include "Traversal.h"
#include "GraphicPipeline.h"
#include "VulkanPipelineBase.h"

namespace rt {
class RayTracingPass : private NonCopyable {
public:
    RayTracingPass(vks::VulkanDevice *vulkandevice, uint32_t width, uint32_t height);
    ~RayTracingPass() noexcept;

    bool InitTraversal();
    bool BuildBVH(vkglTF::Model &scene, const glm::mat4 &modelMatrix);
    void UpdateBVH(vkglTF::Model &scene, const glm::mat4 &modelMatrix);
    bool SetupRenderPass();
    bool SetupDepthOnlyPipeline(
        const vkpip::ExtraPipelineResources &resource,
        const std::vector<VkDescriptorSetLayout> &setLayouts,
        const std::vector<VkPushConstantRange> &pushConstantRanges,
        VkPipelineCache pipelineCache = VK_NULL_HANDLE);
    bool SetupRayTracingPipeline(
        const std::string& shaderName,
        const vkpip::ExtraPipelineResources &resource,
        const std::vector<VkDescriptorSetLayout> &setLayouts,
        const std::vector<VkPushConstantRange> &pushConstantRanges,
        VkPipelineCache pipelineCache = VK_NULL_HANDLE);

    void UpdateMatrices(const buf::UBOMatrices &uboMatrices);
    void UpdateParams(const buf::UBOParams &uboParams);

    const VkDescriptorImageInfo &GetColorAttachmentImageInfo() const
    {
        return m_reflTexDescInfo;
    }

    void RefitBVH(VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
    void Draw(VkCommandBuffer cmdBuffer, vkibl::VulkanImageBasedLighting *ibl, vkglTF::Model &scene);
    float GetReflectArea() const;
    void SetStat(bool stat) { m_showStat = stat; }

private:
    void SetupUniformBuffers();
    bool CreateRayTracingDescSet();

    VkRenderPassBeginInfo BuildRenderPassBeginInfo(const std::vector<VkClearValue> &clearValues);
    void SetViewport(VkCommandBuffer cmdBuffer);

    void RecordDepthOnlyCmd(VkCommandBuffer cmdBuffer, vkglTF::Model &scene);
    void RecordRayTracingCmd(VkCommandBuffer cmdBuffer, vkibl::VulkanImageBasedLighting *ibl, vkglTF::Model &scene);

    vks::VulkanDevice *m_vulkandevice = nullptr;
    std::unique_ptr<vks::Framebuffer> m_framebuffer;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    VkDescriptorImageInfo m_reflTexDescInfo {};

    struct UniformBufferSet {
        vks::Buffer matrices;
        vks::Buffer params;
    } m_uniformBuffers;

    std::vector<vkvert::Vertex> m_worldVertices;
    std::unique_ptr<RayShop::Vulkan::Traversal> m_traversal;
    // BVH vertices and indices
    struct BVHBuffers {
        vks::Buffer vertex;
        vks::Buffer index;
    } m_bvhBuffers;
    struct BVHStagingBuffers {
        vks::Buffer vertex;
        vks::Buffer index;
    } m_bvhStagingBuffers;
    // count
    static constexpr uint32_t m_countSize = 4;
    vks::Buffer m_countBuffer;
    vks::Buffer m_countStagingBuffer;
    std::vector<RayShop::GeometryTriangleDescription> m_bvhGeometriesOnCPU;
    std::vector<RayShop::GeometryTriangleDescription> m_bvhGeometriesOnGPU;
    std::vector<RayShop::BLAS> m_blases;

    std::unique_ptr<GraphicPipeline> m_depthOnlyPipeline;
    std::unique_ptr<DescSet> m_depthOnlyDescSet;

    std::unique_ptr<GraphicPipeline> m_rtPipeline;
    std::unique_ptr<DescSet> m_rtDescSet;

    bool m_showStat = false;
};
} // namespace rt
#endif // VULKANEXAMPLES_RAYTRACINGPASS_H
