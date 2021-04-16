/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Pipeline factory class
 */

#ifndef VULKANEXAMPLES_VULKANPIPELINEFACTORY_H
#define VULKANEXAMPLES_VULKANPIPELINEFACTORY_H
#include "VulkanPipelineCreateFuncMgr.h"

namespace vkpip {
class VulkanPipelineFactory {
public:
    static RenderPipelinePtr MakePipelineInstance(RenderPipelineType type, vks::VulkanDevice *vulkanDevice,
                                                  const ExtraPipelineResources *resources,
                                                  const std::string vertShaderName,
                                                  const std::string fragShaderName = "")
    {
        auto createFunc = VulkanPipelineCreateFuncMgr::Instance().GetFunc(type);
        if (createFunc == nullptr) {
            return nullptr;
        }

        return createFunc(vulkanDevice, resources, vertShaderName, fragShaderName);
    }
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANPIPELINEFACTORY_H
