/*
 * Vulkan texture loader
 *
 * Copyright(C) by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"

#include <ktx.h>
#include <ktxvulkan.h>


#include "tiny_gltf.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTools.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#include "VulkanAndroid.h"
#endif

namespace vks {
class Texture {
public:
    vks::VulkanDevice *device;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory deviceMemory;
    VkImageView view;
    uint32_t width, height;
    uint32_t mipLevels;
    uint32_t layerCount;
    VkDescriptorImageInfo descriptor;
    VkSampler sampler = VK_NULL_HANDLE;
    struct {
        VkImageCreateInfo imageCreateInfo;
        bool prepare = false;
    }imageCreateInfors;
    struct {
        VkImageViewCreateInfo imageViewCreateInfo;
        bool prepare = false;
    }imageViewCreateInfors;
    struct {
        VkSamplerCreateInfo samplerCreateInfo;
        bool prepare = false;
    }samplerCreateInfors;




    void updateDescriptor();
    void destroy();
    ktxResult loadKTXFile(std::string filename, ktxTexture **target);
    void setupInitilaImageCreateInfor(VkFormat format,
                                      uint32_t texWidth,
                                      uint32_t texHeight,
                                      uint32_t numMips = 1,
                                      uint32_t arrayLayers = 1,
                                      VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT);
    void setupInitialImageViewCreateInfor(VkFormat format, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t levelCount = 1, uint32_t layerCount = 1);
    void setupInitialSamplerCreateInfor(VkFilter filter = VK_FILTER_LINEAR, uint32_t maxLod = 1);
    void createTexture(vks::VulkanDevice *device, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool createSampler = true);
};

struct TextureSampler {
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
};

class Texture2D : public Texture {
public:
    void loadFromFile(std::string filename, VkFormat format, vks::VulkanDevice *device, VkQueue copyQueue,
                      VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
                      VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool forceLinear = false);
    void fromglTfImage(tinygltf::Image &gltfimage, std::string path, vks::VulkanDevice *device, VkQueue copyQueue);
    void fromglTfImage(tinygltf::Image &gltfimage, TextureSampler textureSampler, vks::VulkanDevice *device,
                       VkQueue copyQueue);
    void fromBuffer(void *buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight,
                    vks::VulkanDevice *device, VkQueue copyQueue, VkFilter filter = VK_FILTER_LINEAR,
                    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
                    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    void createEmptyTexture(VkFormat format, vks::VulkanDevice *device,VkQueue copyQueue,
                            VkFilter           filter          = VK_FILTER_LINEAR,
                            VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                            VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};

class Texture2DArray : public Texture {
public:
    void loadFromFile(std::string filename, VkFormat format, vks::VulkanDevice *device, VkQueue copyQueue,
                      VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
                      VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};

class TextureCubeMap : public Texture {
public:
    void loadFromFile(std::string filename, VkFormat format, vks::VulkanDevice *device, VkQueue copyQueue,
                      VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
                      VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};
} // namespace vks
