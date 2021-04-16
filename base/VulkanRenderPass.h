/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan Renderpass header
 */

#ifndef VULKANEXAMPLES_VULKANRENDERPASS_H
#define VULKANEXAMPLES_VULKANRENDERPASS_H
#include <array>
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"
namespace vkpass {
class VulkanRenderPass {
public:
    vks::VulkanDevice *device;
    struct RenderpassCreateAttributes {
        std::vector<VkAttachmentDescription> attachmentDescriptions;
        VkSubpassDescription subpassDescription;
        std::vector<VkAttachmentReference> colorReferences;
        VkAttachmentReference depthReference{};
        std::array<VkSubpassDependency, 2> dependencies;
    } renderpassCreateAttributes;
    VkRenderPassCreateInfo renderPassCreateInfo;
    VkRenderPass renderpass = VK_NULL_HANDLE;
    bool prepare = false;

    VulkanRenderPass(vks::VulkanDevice *device) : device(device){};
    ~VulkanRenderPass();
    void prepareRenderPass();
    void addAttachmentDescriptions(VkFormat format, VkImageLayout finalLayout);
    void createRenderPass();

protected:
    void setRenderpassCreateInfor();
    void setRenderpassAttributes();
};
} // namespace vkpass

#endif // VULKANEXAMPLES_VULKANRENDERPASS_H
