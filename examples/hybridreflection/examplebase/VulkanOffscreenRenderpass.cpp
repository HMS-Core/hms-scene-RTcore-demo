/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan offscreen render pass implementation file
 */

#include "VulkanOffscreenRenderpass.h"

namespace vkpass {
VulkanOffscreenRenderpass::~VulkanOffscreenRenderpass()
{
    frameBuffers.generateRay = nullptr;
    frameBuffers.reflection = nullptr;
}

void VulkanOffscreenRenderpass::buildOffscreenRenderpass(
    vkglTF::Model *scene, std::map<vkpip::RenderPipelineType, RenderPipelinePtr &> *pipelines)
{
    if (commandBuffers.generateRay == VK_NULL_HANDLE) {
        commandBuffers.generateRay = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
    }

    if (commandBuffers.reflection == VK_NULL_HANDLE) {
        commandBuffers.reflection = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
    }

    // Clear values for all attachments written in the fragment shader
    std::array<VkClearValue, 2> clearValues;

    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();

    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.pClearValues = clearValues.data();

    VkViewport viewport = vks::initializers::viewport(
        static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
    // Generate ray command buffer and render pass
    {
        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers.generateRay, &cmdBufInfo));
        clearValues[0].depthStencil = {1.0f, 0};
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.renderPass = frameBuffers.generateRay->renderPass;
        renderPassBeginInfo.framebuffer = frameBuffers.generateRay->framebuffer;

        vkCmdBeginRenderPass(commandBuffers.generateRay, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffers.generateRay, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffers.generateRay, 0, 1, &scissor);
        // Depth only
        pipelines->at(vkpip::DEPTH_ONLY_PIPELINE)->draw(commandBuffers.generateRay, scene);
        // Generate ray
        pipelines->at(vkpip::GENERATE_RAY_PIPELINE)->draw(commandBuffers.generateRay, scene);
        vkCmdEndRenderPass(commandBuffers.generateRay);
        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers.generateRay));
    }

    // reflection draw command buffer and render pass
    {
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.renderPass = frameBuffers.reflection->renderPass;
        renderPassBeginInfo.framebuffer = frameBuffers.reflection->framebuffer;

        VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers.reflection, &cmdBufInfo));
        vkCmdBeginRenderPass(commandBuffers.reflection, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffers.reflection, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffers.reflection, 0, 1, &scissor);

        // Reflection
        pipelines->at(vkpip::REFLECT_RENDER_PIPELINE)->draw(commandBuffers.reflection, scene);
        vkCmdEndRenderPass(commandBuffers.reflection);

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers.reflection));
    }
}

void VulkanOffscreenRenderpass::prepareReflectionRenderpass()
{
    // contain: 1 color + 1 depth
    frameBuffers.reflection = std::make_unique<vks::Framebuffer>(device);
    frameBuffers.reflection->width = width;
    frameBuffers.reflection->height = height;

    // Four attachments (1 color, 1 depth)
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.width = width;
    attachmentInfo.height = height;
    attachmentInfo.layerCount = 1;
    attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // Color attachments
    // Attachment 0: color
    attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    frameBuffers.reflection->addAttachment(attachmentInfo);

    // Depth attachment
    // Find a suitable depth format
    VkFormat attDepthFormat;
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(device->physicalDevice, &attDepthFormat);

    attachmentInfo.format = attDepthFormat;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    frameBuffers.reflection->addAttachment(attachmentInfo);

    // Create sampler to sample from the color attachments
    VK_CHECK_RESULT(frameBuffers.reflection->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST,
                                                           VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

    // Create default renderpass for the framebuffer
    VK_CHECK_RESULT(frameBuffers.reflection->createRenderPass(
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));

    reflectionTextureDescriptor = vks::initializers::descriptorImageInfo(frameBuffers.reflection->sampler,
                                                                         frameBuffers.reflection->attachments[0].view,
                                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanOffscreenRenderpass::prepareGenerateRayRenderpass()
{
    frameBuffers.generateRay = std::make_unique<vks::Framebuffer>(device);
    frameBuffers.generateRay->width = width;
    frameBuffers.generateRay->height = height;
    // Depth only
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.width = width;
    attachmentInfo.height = height;
    attachmentInfo.layerCount = 1;

    // Depth attachment
    // Find a suitable depth format
    VkFormat attDepthFormat;
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(device->physicalDevice, &attDepthFormat);
    attachmentInfo.format = attDepthFormat;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    frameBuffers.generateRay->addAttachment(attachmentInfo);

    // Create default renderpass for the framebuffer
    VK_CHECK_RESULT(frameBuffers.generateRay->createRenderPass(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                               VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT));
}

void VulkanOffscreenRenderpass::prepareOffscreenPass()
{
    prepareGenerateRayRenderpass();
    prepareReflectionRenderpass();
}

const VkDescriptorImageInfo *VulkanOffscreenRenderpass::getReflectionTextureDescriptor()
{
    return &reflectionTextureDescriptor;
}
} // namespace vkpass