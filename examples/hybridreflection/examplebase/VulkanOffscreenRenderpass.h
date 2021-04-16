/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan offscreen render pass head file
 */

#ifndef VULKANEXAMPLES_VULKANOFFSCREENRENDERPASS_H
#define VULKANEXAMPLES_VULKANOFFSCREENRENDERPASS_H
#include <array>
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanFrameBuffer.hpp"
#include "VulkanPipelineBase.h"
#include "vector"
#include "common.h"

namespace vkpass {
class VulkanOffscreenRenderpass {
public:
    struct {
        std::unique_ptr<vks::Framebuffer> generateRay;
        std::unique_ptr<vks::Framebuffer> reflection;
    } frameBuffers;

    struct {
        VkCommandBuffer generateRay = VK_NULL_HANDLE;
        VkCommandBuffer reflection = VK_NULL_HANDLE;
    } commandBuffers;

    VulkanOffscreenRenderpass(vks::VulkanDevice *vulkanDevice, uint32_t frameBufferWidth, uint32_t frameBufferHeight)
        : device(vulkanDevice), width(frameBufferWidth), height(frameBufferHeight){};
    virtual ~VulkanOffscreenRenderpass();

    void buildOffscreenRenderpass(vkglTF::Model *scene, std::map<vkpip::RenderPipelineType,
        RenderPipelinePtr&>* pipelines);

    // Prepare the framebuffer for offscreen rendering with multiple
    // attachments used as render targets inside the fragment shaders
    void prepareOffscreenPass();
    const VkDescriptorImageInfo *getReflectionTextureDescriptor();

private:
    vks::VulkanDevice *device = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    VkDescriptorImageInfo reflectionTextureDescriptor;

    void prepareReflectionRenderpass();
    void prepareGenerateRayRenderpass();
};
} // namespace vkpass

#endif // VULKANEXAMPLES_VULKANOFFSCREENRENDERPASS_H
