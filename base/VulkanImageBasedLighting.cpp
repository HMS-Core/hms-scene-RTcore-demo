/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan image based lighting implementation file
 */

#include "VulkanImageBasedLighting.h"
#include "VulkanIBLPipelines.h"
#include "VulkanRenderPass.h"
constexpr int CUBU_TEXTURE_NUM = 6;

VkDescriptorSetLayout vkibl::descriptorSetLayoutImage = VK_NULL_HANDLE;

void CreateFrameBuffer(VkFramebuffer &framebuffer, VkDevice logicalDevice, VkRenderPass renderPass,
                       VkImageView imageView, uint32_t dim)
{
    VkFramebufferCreateInfo framebufferCI = vks::initializers::framebufferCreateInfo();
    framebufferCI.renderPass = renderPass;
    framebufferCI.attachmentCount = 1;
    framebufferCI.pAttachments = &imageView;
    framebufferCI.width = dim;
    framebufferCI.height = dim;
    framebufferCI.layers = 1;
    VK_CHECK_RESULT(vkCreateFramebuffer(logicalDevice, &framebufferCI, nullptr, &framebuffer));
}

void BuildCopyImageCommand(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstCube, uint32_t baseArrayLayer,
                           uint32_t mipLevel, uint32_t copyWidth, uint32_t copyHeight)
{
    vks::tools::setImageLayout(commandBuffer, srcImage, VK_IMAGE_ASPECT_COLOR_BIT,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Copy region for transfer from framebuffer to cube face
    VkImageCopy copyRegion = {};

    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = {0, 0, 0};

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = baseArrayLayer;
    copyRegion.dstSubresource.mipLevel = mipLevel;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = {0, 0, 0};

    copyRegion.extent.width = static_cast<uint32_t>(copyWidth);
    copyRegion.extent.height = static_cast<uint32_t>(copyHeight);
    copyRegion.extent.depth = 1;

    vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstCube,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // Transform framebuffer color attachment back
    vks::tools::setImageLayout(commandBuffer, srcImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

glm::mat4 GetMVP(uint32_t faceIndex)
{
    std::vector<glm::mat4> matrices = {
        // POSITIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    };

    glm::mat4 mvp = glm::perspective(static_cast<float>((M_PI / 2.0)), 1.0f, 0.1f, 512.0f) * matrices[faceIndex];
    return mvp;
}

struct PushBlock {
    glm::mat4 mvp;
    union {
        struct {
            float roughness;
            uint32_t numSamples;
        };
        struct {
            // Sampling deltas
            float deltaPhi;
            float deltaTheta;
        };
    };
};

struct RenderCubeConfigs {
    vks::TextureCubeMap *environmentCube;
    vkglTF::Model *skybox;
    VkFramebuffer framebuffer;
    vks::TextureCubeMap *dstCubeTexture;
    vks::Texture2D *srcTexture2D;
    uint32_t numMips;
    PushBlock pushBlock;
    std::string vertShaderName = "hybridRayTracing/filtercube.vert.spv";
    std::string fragShaderName;
    bool preFilter = false;
};

struct RenderPassBeginInfors {
    VkRenderPassBeginInfo renderPassBeginInfo;
    VkViewport viewport;
    VkRect2D scissor;
};

RenderPassBeginInfors InitialPassBeginInfor(VkRenderPass renderPass, VkFramebuffer framebuffer,
                                            VkCommandBuffer commandBuffer, uint32_t width, uint32_t height)
{
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};
    VkViewport viewport = vks::initializers::viewport(static_cast<float>(width), static_cast<float>(height),
        0.0f, 1.0f);
    VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();

    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    RenderPassBeginInfors beginInfors = {renderPassBeginInfo, viewport, scissor};
    return beginInfors;
}

void RenderCubeTextures(vks::VulkanDevice *device, VkQueue queue, VkRenderPass renderPass,
                        VkPipelineCache pipelineCache, RenderCubeConfigs renderCubeConfigs)
{
    std::vector<VkPushConstantRange> pushConstantRanges = {
        vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                             sizeof(PushBlock), 0),
    };
    vkpip::ExtraPipelineResources resources;
    resources.textureCubeMap = renderCubeConfigs.environmentCube;
    vkpip::VulkanRenderCubePipeline renderCubePipeline(
        device, &resources, {renderCubeConfigs.vertShaderName, renderCubeConfigs.fragShaderName});
    renderCubePipeline.PreparePipelines(renderPass, pipelineCache, nullptr, &pushConstantRanges, 0);
    VkCommandBuffer cmdBuf = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    RenderPassBeginInfors beginInfors = InitialPassBeginInfor(renderPass, renderCubeConfigs.framebuffer, cmdBuf,
                                                              renderCubeConfigs.srcTexture2D->width,
                                                              renderCubeConfigs.srcTexture2D->height);
    uint32_t dim = renderCubeConfigs.srcTexture2D->width;
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = renderCubeConfigs.numMips;
    subresourceRange.layerCount = CUBU_TEXTURE_NUM;
    // Change image layout for all cubemap faces to transfer destination
    vks::tools::setImageLayout(cmdBuf, renderCubeConfigs.dstCubeTexture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    for (uint32_t m = 0; m < renderCubeConfigs.numMips; m++) {
        if (renderCubeConfigs.preFilter) {
            renderCubeConfigs.pushBlock.roughness = static_cast<float>(m) / static_cast<float>(
                renderCubeConfigs.numMips - 1);
        }
        for (uint32_t f = 0; f < CUBU_TEXTURE_NUM; f++) {
            beginInfors.viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
            beginInfors.viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
            vkCmdSetViewport(cmdBuf, 0, 1, &beginInfors.viewport);
            // Render scene from cube face's point of view
            vkCmdBeginRenderPass(cmdBuf, &beginInfors.renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            // Update shader push constant block
            renderCubeConfigs.pushBlock.mvp = GetMVP(f);

            vkCmdPushConstants(cmdBuf, renderCubePipeline.pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock),
                               &renderCubeConfigs.pushBlock);
            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderCubePipeline.pipeline);
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderCubePipeline.pipelineLayout, 0, 1,
                                    &renderCubePipeline.descriptorSet, 0, NULL);
            renderCubeConfigs.skybox->draw(cmdBuf);
            vkCmdEndRenderPass(cmdBuf);
            BuildCopyImageCommand(cmdBuf, renderCubeConfigs.srcTexture2D->image,
                                  renderCubeConfigs.dstCubeTexture->image, f, m,
                                  static_cast<uint32_t>(beginInfors.viewport.width),
                                  static_cast<uint32_t>(beginInfors.viewport.height));
        }
    }

    vks::tools::setImageLayout(cmdBuf, renderCubeConfigs.dstCubeTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    device->flushCommandBuffer(cmdBuf, queue);
}

vkibl::VulkanImageBasedLighting::~VulkanImageBasedLighting() noexcept
{
    if (descriptorSetLayoutImage != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayoutImage, nullptr);
        descriptorSetLayoutImage = VK_NULL_HANDLE;
    }
    vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    textures.irradianceCube.destroy();
    textures.prefilteredCube.destroy();
    textures.lutBrdf.destroy();
    environmentCube->destroy();
    descriptorSet = nullptr;
    skybox = nullptr;
    device = nullptr;
}

void vkibl::VulkanImageBasedLighting::GenerateBRDFLUT(VkPipelineCache pipelineCache, VkQueue queue)
{
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format = VK_FORMAT_R16G16_SFLOAT; // R16G16 is supported pretty much everywhere
    const int32_t dim = configs.brdfLutDim;
    textures.lutBrdf.setupInitilaImageCreateInfor(format, dim, dim, 1, 1,
                                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    textures.lutBrdf.setupInitialImageViewCreateInfor(format);
    textures.lutBrdf.setupInitialSamplerCreateInfor();
    textures.lutBrdf.createTexture(device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);

    // FB, Att, RP, Pipe, etc.
    vkpass::VulkanRenderPass vulkanRenderPass(device);
    vulkanRenderPass.AddAttachmentDescriptions(VK_FORMAT_R16G16_SFLOAT,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vulkanRenderPass.PrepareRenderPass();
    vulkanRenderPass.CreateRenderPass();

    VkFramebuffer framebuffer;
    CreateFrameBuffer(framebuffer, device->logicalDevice, vulkanRenderPass.renderpass, textures.lutBrdf.view, dim);
    vkpip::VulkanGenBRDFFlutPipeline genBrdfFlutPipeline(
        device, nullptr, {"hybridRayTracing/genbrdflut.vert.spv", "hybridRayTracing/genbrdflut.frag.spv"});
    genBrdfFlutPipeline.PreparePipelines(vulkanRenderPass.renderpass, pipelineCache, nullptr,
                                         nullptr, 0);

    // Render
    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = vulkanRenderPass.renderpass;
    renderPassBeginInfo.renderArea.extent.width = dim;
    renderPassBeginInfo.renderArea.extent.height = dim;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = framebuffer;

    VkCommandBuffer cmdBuf = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = vks::initializers::viewport(static_cast<float>(dim), static_cast<float>(dim), 0.0f, 1.0f);
    VkRect2D scissor = vks::initializers::rect2D(dim, dim, 0, 0);
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, genBrdfFlutPipeline.pipeline);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);
    device->flushCommandBuffer(cmdBuf, queue);

    vkQueueWaitIdle(queue);

    vkDestroyFramebuffer(device->logicalDevice, framebuffer, nullptr);

    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating BRDF LUT took " << tDiff << " ms" << std::endl;
}

struct Offscreen {
    vks::Texture2D texture;
    VkFramebuffer framebuffer;
    void PrepareOffscreen(vks::VulkanDevice *device, VkRenderPass renderPass, VkQueue queue, VkFormat format,
                          uint32_t dim)
    {
        texture.setupInitilaImageCreateInfor(format, dim, dim, 1, 1,
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        texture.imageCreateInfors.imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        texture.setupInitialImageViewCreateInfor(format);
        texture.imageViewCreateInfors.imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        texture.imageViewCreateInfors.imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        texture.createTexture(device, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);

        CreateFrameBuffer(framebuffer, device->logicalDevice, renderPass, texture.view, dim);

        VkCommandBuffer layoutCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        vks::tools::setImageLayout(layoutCmd, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        device->flushCommandBuffer(layoutCmd, queue, true);
    }
};
// Offfscreen framebuffer

// Generate an irradiance cube map from the environment cube map
void vkibl::VulkanImageBasedLighting::GenerateIrradianceCube(VkPipelineCache pipelineCache, VkQueue queue)
{
    auto tStart = std::chrono::high_resolution_clock::now();

    const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    const int32_t dim = configs.irradianceCubeDim;
    const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

    // Pre-filtered cube map
    textures.irradianceCube.setupInitilaImageCreateInfor(format, dim, dim, numMips, CUBU_TEXTURE_NUM,
                                                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    textures.irradianceCube.imageCreateInfors.imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    textures.irradianceCube.setupInitialImageViewCreateInfor(format, VK_IMAGE_VIEW_TYPE_CUBE, numMips,
                                                             CUBU_TEXTURE_NUM);
    textures.irradianceCube.setupInitialSamplerCreateInfor(VK_FILTER_LINEAR, numMips);
    textures.irradianceCube.createTexture(device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);

    // FB, Att, RP, Pipe, etc.
    vkpass::VulkanRenderPass vulkanRenderPass(device);
    vulkanRenderPass.AddAttachmentDescriptions(format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vulkanRenderPass.PrepareRenderPass();
    vulkanRenderPass.CreateRenderPass();
    Offscreen offscreen;
    offscreen.PrepareOffscreen(device, vulkanRenderPass.renderpass, queue, format, dim);
    PushBlock pushBlock;
    pushBlock.deltaPhi = (2.0f * static_cast<float>(M_PI)) / 180.0f;
    pushBlock.deltaTheta = (0.5f * static_cast<float>(M_PI)) / 64.0f;

    RenderCubeConfigs renderCubeConfigs = {environmentCube,
                                           skybox,
                                           offscreen.framebuffer,
                                           &textures.irradianceCube,
                                           &offscreen.texture,
                                           numMips,
                                           pushBlock,
                                           "hybridRayTracing/filtercube.vert.spv",
                                           "hybridRayTracing/irradiancecube.frag.spv"};

    RenderCubeTextures(device, queue, vulkanRenderPass.renderpass, pipelineCache, renderCubeConfigs);
    vkDestroyFramebuffer(device->logicalDevice, offscreen.framebuffer, nullptr);
    offscreen.texture.destroy();
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating irradiance cube with " << numMips << " mip levels took " << tDiff << " ms" << std::endl;
}

// Prefilter environment cubemap
// See
// https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
void vkibl::VulkanImageBasedLighting::GeneratePrefilteredCube(VkPipelineCache pipelineCache, VkQueue queue)
{
    auto tStart = std::chrono::high_resolution_clock::now();
    const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    const int32_t dim = 512;
    const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
    prefilteredCubeMipLevels = static_cast<float>(numMips);
    textures.prefilteredCube.setupInitilaImageCreateInfor(format, dim, dim, numMips, CUBU_TEXTURE_NUM,
                                                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    textures.prefilteredCube.imageCreateInfors.imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    textures.prefilteredCube.setupInitialImageViewCreateInfor(format, VK_IMAGE_VIEW_TYPE_CUBE, numMips,
                                                              CUBU_TEXTURE_NUM);
    textures.prefilteredCube.setupInitialSamplerCreateInfor(VK_FILTER_LINEAR, numMips);
    textures.prefilteredCube.createTexture(device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);

    // Pre-filtered cube map
    vkpass::VulkanRenderPass vulkanRenderPass(device);
    vulkanRenderPass.AddAttachmentDescriptions(format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vulkanRenderPass.PrepareRenderPass();
    vulkanRenderPass.CreateRenderPass();
    Offscreen offscreen;
    offscreen.PrepareOffscreen(device, vulkanRenderPass.renderpass, queue, format, dim);
    PushBlock pushBlock;
    pushBlock.numSamples = 32u;
    RenderCubeConfigs renderCubeConfigs = {environmentCube,
                                           skybox,
                                           offscreen.framebuffer,
                                           &textures.prefilteredCube,
                                           &offscreen.texture,
                                           numMips,
                                           pushBlock,
                                           "hybridRayTracing/filtercube.vert.spv",
                                           "hybridRayTracing/prefilterenvmap.frag.spv",
                                           true};
    RenderCubeTextures(device, queue, vulkanRenderPass.renderpass, pipelineCache, renderCubeConfigs);
    vkDestroyFramebuffer(device->logicalDevice, offscreen.framebuffer, nullptr);
    offscreen.texture.destroy();
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating pre-filtered enivornment cube with " << numMips << " mip levels took " << tDiff << " ms"
              << std::endl;
}

void vkibl::VulkanImageBasedLighting::SetupDescriptors()
{
    // 0: samplerIrradiance
    // 1: prefilteredMap
    // 2: samplerBRDFLUT
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}};
    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCI.pPoolSizes = poolSizes.data();
    descriptorPoolCI.maxSets = 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool));

    /*
        Descriptor sets
    */

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
    descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorSetLayoutCI, nullptr, &descriptorSetLayoutImage));

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayoutImage;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &descriptorSet));

    std::vector<VkDescriptorImageInfo> imageDescriptors = {
        textures.irradianceCube.descriptor, textures.prefilteredCube.descriptor, textures.lutBrdf.descriptor};

    std::array<VkWriteDescriptorSet, 3> writeDescriptorSets{};
    for (size_t i = 0; i < imageDescriptors.size(); i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].descriptorCount = 1;
        writeDescriptorSets[i].dstSet = descriptorSet;
        writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
        writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
    }

    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);
}

void vkibl::VulkanImageBasedLighting::GenerateIBLTextures(VkPipelineCache pipelineCache, VkQueue queue)
{
    this->GenerateBRDFLUT(pipelineCache, queue);
    this->GenerateIrradianceCube(pipelineCache, queue);
    this->GeneratePrefilteredCube(pipelineCache, queue);
    this->SetupDescriptors();
}

void vkibl::VulkanImageBasedLighting::BindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                                                        uint32_t bindImageSet)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindImageSet, 1,
                            &descriptorSet, 0, nullptr);
}