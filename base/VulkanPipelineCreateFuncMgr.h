/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Pipeline creation manage class header
 */
#ifndef VULKANEXAMPLES_VULKANPIPELINECREATEFUNCMGR_H
#define VULKANEXAMPLES_VULKANPIPELINECREATEFUNCMGR_H
#include <map>
#include <functional>
#include "VulkanPipelineBase.h"

namespace vkpip {
using RenderPipelineCreateFunc = std::function<RenderPipelinePtr(
    vks::VulkanDevice *, const ExtraPipelineResources *, const PipelineShaderCreateInfor &)>;
class VulkanPipelineCreateFuncMgr {
public:
    static VulkanPipelineCreateFuncMgr &Instance();
    void RegFunc(RenderPipelineType type, const RenderPipelineCreateFunc &func)
    {
        container[type] = func;
    }

    RenderPipelineCreateFunc GetFunc(RenderPipelineType type)
    {
        auto it = container.find(type);
        if (it == container.end()) {
            return nullptr;
        }

        return it->second;
    }

private:
    std::map<RenderPipelineType, RenderPipelineCreateFunc> container{};

    VulkanPipelineCreateFuncMgr() = default;
    ~VulkanPipelineCreateFuncMgr() = default;
};

class VulkanPipelineCreateFuncRegister {
public:
    VulkanPipelineCreateFuncRegister(RenderPipelineType type, const RenderPipelineCreateFunc &func)
    {
        VulkanPipelineCreateFuncMgr::Instance().RegFunc(type, func);
    }

    ~VulkanPipelineCreateFuncRegister() = default;
};

template<typename PipelineRegClassType>
inline void RegRenderPipeline(RenderPipelineType type)
{
    static VulkanPipelineCreateFuncRegister regPipeline(type,
        [] (vks::VulkanDevice *vulkanDevice, const vkpip::ExtraPipelineResources *resources,
            const PipelineShaderCreateInfor &pipelineShaderCreateInfor)
        {
            return std::make_unique<PipelineRegClassType>(
                vulkanDevice,
                resources,
                pipelineShaderCreateInfor);
        }
    );
}
} // namespace vkpip

#endif // VULKANEXAMPLES_VULKANPIPELINECREATEFUNCMGR_H
