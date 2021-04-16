/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan image based lighting head file
 */

#ifndef VULKANEXAMPLES_VULKANIMAGEBASEDLIGHTING_H
#define VULKANEXAMPLES_VULKANIMAGEBASEDLIGHTING_H
#include <chrono>

#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"
#include "SaschaWillemsVulkan/VulkanTexture.h"
#include "SaschaWillemsVulkan/VulkanTools.h"
#include "SaschaWillemsVulkan/VulkanglTFModel.h"
#include "SaschaWillemsVulkan/VulkanFrameBuffer.hpp"
#include "VulkanShader.h"
#include "VulkanVertex.h"

namespace vkibl {
extern VkDescriptorSetLayout descriptorSetLayoutImage;

struct IBLConfig {
    uint32_t brdfLutDim = 512;
    uint32_t irradianceCubeDim = 64;
};

class VulkanImageBasedLighting {
public:
    vks::VulkanDevice *device = nullptr;
    vkglTF::Model *skybox = nullptr;
    vks::TextureCubeMap *environmentCube = nullptr;
    float prefilteredCubeMipLevels = 0;
    VulkanImageBasedLighting(vks::VulkanDevice *device, vkglTF::Model *skybox, vks::TextureCubeMap *environmentCube)
        : device(device), skybox(skybox), environmentCube(environmentCube){};
    ~VulkanImageBasedLighting();

    void generateIBLTextures(VkPipelineCache pipelineCache, VkQueue queue);

    void bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bindImageSet);

private:
    struct Textures {
        // Generated at runtime
        vks::Texture2D lutBrdf;
        vks::TextureCubeMap irradianceCube;
        vks::TextureCubeMap prefilteredCube;
    } textures;
    IBLConfig configs;
    VkDescriptorPool descriptorPool;
    // descriptor set for 0: lutBrdf, 1: irradianceCube, 2：prefilteredCube
    VkDescriptorSet descriptorSet;

    // Generate a BRDF integration map used as a look-up-table (stores roughness / NdotV)
    void generateBRDFLUT(VkPipelineCache pipelineCache, VkQueue queue);
    // Generate an irradiance cube map from the environment cube map
    void generateIrradianceCube(VkPipelineCache pipelineCache, VkQueue queue);
    // Prefilter environment cubemap
    // See https://placeholderart.wordpress.com
    // /2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
    void generatePrefilteredCube(VkPipelineCache pipelineCache, VkQueue queue);
    // Alloc Descriptorpool,
    // setup descriptor layout for
    // 0: samplerIrradiance
    // 1: prefilteredMap
    // 2: samplerBRDFLUT
    // setup the  Descriptorset
    void setupDescriptors();
};
} // namespace vkibl

#endif // VULKANEXAMPLES_VULKANIMAGEBASEDLIGHTING_H
