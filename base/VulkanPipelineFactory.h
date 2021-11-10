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
                                                  const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
    {
        auto createFunc = VulkanPipelineCreateFuncMgr::Instance().GetFunc(type);
        if (createFunc == nullptr) {
            return nullptr;
        }

        return createFunc(vulkanDevice, resources, pipelineShaderCreateInfor);
    }
};
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANPIPELINEFACTORY_H
