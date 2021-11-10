/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan shader head file
 */

#ifndef VULKANEXAMPLES_VULKANSHADER_H
#define VULKANEXAMPLES_VULKANSHADER_H
#include <vector>
#include <string>
#include "vulkan/vulkan.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"
#if defined(__ANDROID__)
#include "SaschaWillemsVulkan/VulkanAndroid.h"
#endif

namespace vks {
class VulkanShader {
public:
    static VulkanShader &Instance()
    {
        static VulkanShader inst;
        return inst;
    }
    void destroy(vks::VulkanDevice *device)
    {
        for (auto &shaderModule : shaderModules) {
            vkDestroyShaderModule(device->logicalDevice, shaderModule, nullptr);
        }
        shaderModules.clear();
    }
    VulkanShader() = default;
    virtual ~VulkanShader() = default;
    /** @brief Loads a SPIR-V shader file for the given shader stage */
    VkPipelineShaderStageCreateInfo LoadShader(vks::VulkanDevice *device, std::string fileName,
                                               VkShaderStageFlagBits stage);
    std::string GetShadersPath() const;
private:
    // List of shader modules created (stored for cleanup)
    std::vector<VkShaderModule> shaderModules;
    std::string shaderDir = "glsl";
};

class LoadShaders {
public:
    static VkPipelineShaderStageCreateInfo LoadShader(vks::VulkanDevice *device, std::string fileName,
                                                      VkShaderStageFlagBits stage)
    {
        return VulkanShader::Instance().LoadShader(device, fileName, stage);
    }
    static void Destroy(vks::VulkanDevice *device)
    {
        return VulkanShader::Instance().destroy(device);
    }

    LoadShaders() = default;
    ~LoadShaders() = default;
};
} // namespace vks

#endif // VULKANEXAMPLES_VULKANSHADER_H
