/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan Renderpass implement
 */

#include "VulkanRenderPass.h"
#include "Log.h"
namespace vkpass {
bool HasDepth(VkFormat format)
{
    std::vector<VkFormat> formats = {
        VK_FORMAT_D16_UNORM,         VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,   VK_FORMAT_D32_SFLOAT_S8_UINT,
    };
    return std::find(formats.begin(), formats.end(), format) != std::end(formats);
}

bool HasStencil(VkFormat format)
{
    std::vector<VkFormat> formats = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };
    return std::find(formats.begin(), formats.end(), format) != std::end(formats);
}

bool IsDepthStencil(VkFormat format)
{
    return (HasDepth(format) || HasStencil(format));
}

VulkanRenderPass::~VulkanRenderPass()
{
    if (renderpass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device->logicalDevice, renderpass, nullptr);
    }
    device = nullptr;
}

void VulkanRenderPass::SetRenderpassAttributes()
{
    // Collect attachment references
    bool hasDepth = false;
    bool hasColor = false;
    uint32_t attachmentIndex = 0;
    for (auto &attachment : renderpassCreateAttributes.attachmentDescriptions) {
        if (IsDepthStencil(attachment.format)) {
            // Only one depth attachment allowed
            ASSERT(!hasDepth);
            renderpassCreateAttributes.depthReference.attachment = attachmentIndex;
            renderpassCreateAttributes.depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepth = true;
        } else {
            renderpassCreateAttributes.colorReferences.push_back(
                {attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            hasColor = true;
        }
        attachmentIndex++;
    };

    // Default render pass setup uses only one subpass
    renderpassCreateAttributes.subpassDescription = {};
    renderpassCreateAttributes.subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (hasColor) {
        renderpassCreateAttributes.subpassDescription.pColorAttachments =
            renderpassCreateAttributes.colorReferences.data();
        renderpassCreateAttributes.subpassDescription.colorAttachmentCount =
            static_cast<uint32_t>(renderpassCreateAttributes.colorReferences.size());
    }
    if (hasDepth) {
        renderpassCreateAttributes.subpassDescription.pDepthStencilAttachment =
            &renderpassCreateAttributes.depthReference;
    }
    // Use subpass dependencies for layout transitions
    renderpassCreateAttributes.dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    renderpassCreateAttributes.dependencies[0].dstSubpass = 0;
    renderpassCreateAttributes.dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    renderpassCreateAttributes.dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    renderpassCreateAttributes.dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    renderpassCreateAttributes.dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderpassCreateAttributes.dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    renderpassCreateAttributes.dependencies[1].srcSubpass = 0;
    renderpassCreateAttributes.dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    renderpassCreateAttributes.dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    renderpassCreateAttributes.dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    renderpassCreateAttributes.dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderpassCreateAttributes.dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    renderpassCreateAttributes.dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
}

void VulkanRenderPass::SetRenderpassCreateInfor()
{
    renderPassCreateInfo = vks::initializers::renderPassCreateInfo();
    renderPassCreateInfo.attachmentCount =
        static_cast<uint32_t>(renderpassCreateAttributes.attachmentDescriptions.size());
    renderPassCreateInfo.pAttachments = renderpassCreateAttributes.attachmentDescriptions.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &renderpassCreateAttributes.subpassDescription;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(renderpassCreateAttributes.dependencies.size());
    renderPassCreateInfo.pDependencies = renderpassCreateAttributes.dependencies.data();
}

void VulkanRenderPass::PrepareRenderPass()
{
    this->SetRenderpassAttributes();
    this->SetRenderpassCreateInfor();
    prepare = true;
}

void VulkanRenderPass::CreateRenderPass()
{
    if (prepare) {
        VK_CHECK_RESULT(vkCreateRenderPass(device->logicalDevice, &renderPassCreateInfo, nullptr, &renderpass));
    }
}

void VulkanRenderPass::AddAttachmentDescriptions(VkFormat format, VkImageLayout finalLayout)
{
    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.format = format;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (IsDepthStencil(format)) {
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    } else {
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = finalLayout;
    renderpassCreateAttributes.attachmentDescriptions.push_back(attachmentDescription);
}
} // namespace vkpass
