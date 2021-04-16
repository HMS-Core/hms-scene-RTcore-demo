/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan shader implementation file
 */

#include "VulkanShader.h"
namespace vks {
VkPipelineShaderStageCreateInfo VulkanShader::loadShader(vks::VulkanDevice *device, std::string fileName,
                                                         VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager,
                                                (getShadersPath() + fileName).c_str(), device->logicalDevice);
#else
    shaderStage.module = vks::tools::loadShader((getShadersPath() + fileName).c_str(), device->logicalDevice);
#endif
    shaderStage.pName = "main";
    shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

std::string VulkanShader::getShadersPath() const
{
    return getAssetPath() + "shaders/" + shaderDir + "/";
}
} // namespace vks