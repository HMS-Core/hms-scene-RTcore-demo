/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Pipeline creation manage class implement
 */
#include "VulkanPipelineCreateFuncMgr.h"

namespace vkpip {
VulkanPipelineCreateFuncMgr &VulkanPipelineCreateFuncMgr::Instance()
{
    static VulkanPipelineCreateFuncMgr inst;
    return inst;
}
} // namespace vkpip