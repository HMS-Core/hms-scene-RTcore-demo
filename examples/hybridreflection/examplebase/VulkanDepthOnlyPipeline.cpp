/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan depth only pipeline implement file
 */
#include "VulkanDepthOnlyPipeline.h"
#include "VulkanPipelineCreateFuncMgr.h"

namespace vkpip {
REG_RENDER_PIPELINE(DEPTH_ONLY_PIPELINE, VulkanDepthOnlyPipeline)
void VulkanDepthOnlyPipeline::draw(VkCommandBuffer commandBuffer, vkglTF::Model *scene)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // set 0:  2 ubo buffers: matrices and params, 1 texture
    uint32_t bindSet = 0;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            nullptr);
    // set 1: not used (ibl texture)
    // set 2: 5 material textures not used here
    // set 3: 1 ubo matrices
    scene->draw(commandBuffer, vkglTF::RenderFlags::BindUbo, pipelineLayout, bindSet + 2, 0);
}
} // namespace vkpip