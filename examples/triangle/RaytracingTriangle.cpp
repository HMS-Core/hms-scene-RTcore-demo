/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan trace ray implementation file
 */

#include "RaytracingTriangle.h"

RaytracingTriangle::~RaytracingTriangle() noexcept
{
    hitBuffer.destroy();
    rayBuffer.destroy();
    vertexBuffer.destroy();
    indexBuffer.destroy();
    stagingBuffer.destroy();
    triangleRender = nullptr;
    traceRay = nullptr;
    copyCommand = nullptr;
}

void RaytracingTriangle::UpdateCamPosition()
{
    screenCoordinates.lookfrom =
        glm::vec3(-camera.position.z * sin(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)),
                  -camera.position.z * sin(glm::radians(camera.rotation.x)),
                  camera.position.z * cos(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)));
    screenCoordinates.lookfrom.y *= -1;
}

void RaytracingTriangle::UpateParams()
{
    uboParams.traceRayHeight = traceRayHeight;
    uboParams.traceRayWidth = traceRayWidth;
    uboParams.framebufferHeight = height;
    uboParams.framebufferWidth = width;
}

void RaytracingTriangle::GetScreenCoordinates(ScreenCoordinates &screen)
{
    UpdateCamPosition();
    glm::vec3 lookat = {0.f, 0.f, 0.f};
    glm::vec3 vup = {0.f, 1.f, 0.f};
    glm::vec3 axisZ = glm::normalize(screen.lookfrom - lookat);
    glm::vec3 axisX = glm::normalize(glm::cross(vup, axisZ));
    glm::vec3 axisY = glm::normalize(glm::cross(axisZ, axisX));

    float halfHeight = 1.0 / camera.matrices.perspective[1][1] * camera.getNearClip();
    float halfWidth = halfHeight * static_cast<float>(width) / static_cast<float>(height);
    screen.horizontal = 2 * halfWidth * axisX;
    screen.vertical = 2 * halfHeight * axisY;
    // left down position of the screen
    screen.start = screen.lookfrom - camera.getNearClip() * axisZ - halfWidth * axisX - halfHeight * axisY;
}

void RaytracingTriangle::GetPixelDir(const float u, const float v, const ScreenCoordinates &screen,
                                     RayShop::Ray &primaryRay)
{
    glm::vec3 target = screen.start + u * screen.horizontal + v * screen.vertical;
    glm::vec3 rayDir = glm::normalize(target - screen.lookfrom);

    primaryRay.dir[0] = rayDir.x;
    primaryRay.dir[1] = rayDir.y;
    primaryRay.dir[2] = rayDir.z;
}

void RaytracingTriangle::UpdateRayBuffers()
{
    // Copy to staging buffer
    VK_CHECK_RESULT(stagingBuffer.map());
    stagingBuffer.invalidate(VK_WHOLE_SIZE, 0);
    // Copy to stagingBuffer
    memcpy(stagingBuffer.mapped, rayDatas.data(), rayCount * sizeof(buf::Ray));
    stagingBuffer.unmap();

    copyCommand = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    // Barrier to ensure that buffer copy is finished before device copy from it
    stagingBuffer.addBufferBarrier(copyCommand, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                   VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    // Copy  to device buffer
    VkBufferCopy copyRegion = {};
    copyRegion.size = rayCount * sizeof(RayShop::Ray);
    vkCmdCopyBuffer(copyCommand, stagingBuffer.buffer, rayBuffer.buffer, 1, &copyRegion);
    vulkanDevice->flushCommandBuffer(copyCommand, queue, true);
}

void RaytracingTriangle::GeneratePrimaryRay()
{
    GetScreenCoordinates(screenCoordinates);
    RayShop::Ray primaryRay;
    primaryRay.origin[0] = screenCoordinates.lookfrom.x;
    primaryRay.origin[1] = screenCoordinates.lookfrom.y;
    primaryRay.origin[2] = screenCoordinates.lookfrom.z;
    primaryRay.tmin = 0.01;
    primaryRay.tmax = MAXRAY_LENGTH;
    uint32_t rayIndex = 0;
    for (uint32_t i = 0; i < traceRayHeight; i++) {
        for (uint32_t j = 0; j < traceRayWidth; j++) {
            GetPixelDir(static_cast<float>(j) / traceRayWidth,
                        static_cast<float>(i) / traceRayHeight,
                        screenCoordinates, primaryRay);
            rayDatas[rayIndex] = primaryRay;
            rayIndex++;
        }
    }
}

void RaytracingTriangle::PrepareVertices()
{
    // Setup vertices
    vertices.clear();
    vkvert::Vertex vertex0, vertex1, vertex2;
    vertex0.pos = {1.0f, 1.0f, 0.0f, 0.f};
    vertex0.color = {1.0f, 0.0f, 0.0f, 1.f};
    vertex1.pos = {-1.0f, 1.0f, 0.0f, 0.f};
    vertex1.color = {0.0f, 1.0f, 0.0f, 1.f};
    vertex2.pos = {0.0f, -1.0f, 0.0f, 0.f};
    vertex2.color = {0.0f, 0.0f, 1.0, 1.f};

    vertices.push_back(vertex0);
    vertices.push_back(vertex1);
    vertices.push_back(vertex2);
    // Setup indices
    indices = {0, 1, 2};
    size_t vertexBufferSize = vertices.size() * sizeof(vkvert::Vertex);
    size_t indexBufferSize = indices.size() * sizeof(uint32_t);
    vulkanDevice->createBufferWithStagigingBuffer(
        queue,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, vertexBufferSize, static_cast<void*>(vertices.data()));
    vulkanDevice->createBufferWithStagigingBuffer(
        queue, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, indexBufferSize, static_cast<void*>(indices.data()));
}

void RaytracingTriangle::PrepareStorageBuffers()
{
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               &stagingBuffer, rayCount * sizeof(RayShop::Ray), nullptr);
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &rayBuffer, rayCount * sizeof(RayShop::Ray),
                               nullptr);
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &hitBuffer,
        rayCount * RayShop::Vulkan::Traversal::GetHitFormatBytes(RayShop::TraceRayHitFormat::T_PRIMID_U_V));
}

void RaytracingTriangle::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    VkClearValue clearValues[2];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    for (size_t i = 0; i < drawCmdBuffers.size(); ++i) {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = frameBuffers[i];
        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport = vks::initializers::viewport(static_cast<float>(width), static_cast<float>(height),
            0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
        VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
        vkpip::PipelineDrawInfor pipelineDrawInfor{3, 0};
        triangleRender->Draw(drawCmdBuffers[i], &pipelineDrawInfor);
        drawUI(drawCmdBuffers[i]);
        vkCmdEndRenderPass(drawCmdBuffers[i]);
        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}

void RaytracingTriangle::UpdateUniformBuffers()
{
    UpateParams();
    triangleRender->UpdateParams(&uboParams);
}

void RaytracingTriangle::PreparePipelines()
{
    resources.hitBuffer = &hitBuffer;
    resources.vertexBuffer = &vertexBuffer;
    resources.indexBuffer = &indexBuffer;
    vkpip::PipelineShaderCreateInfor pipelineShaderCreateInfor = {"triangle/fullscreen.vert.spv",
                                                                  "triangle/showhit.frag.spv"};
    triangleRender =
        std::make_unique<vkpip::VulkanTrianglePipeline>(vulkanDevice, &resources, pipelineShaderCreateInfor);
    triangleRender->PreparePipelines(renderPass, pipelineCache, nullptr, nullptr, 0);
}

void RaytracingTriangle::PrepareRayTracing()
{
    // Trace ray pipeline
    traceRay =
        std::make_unique<rt::VulkanTraceRay>(vulkanDevice, traceRayWidth, traceRayHeight, &hitBuffer, &rayBuffer);
    traceRay->Prepare();
    traceRay->BuildBVH(&vertices, &indices, glm::mat4(1.0f));
}

void RaytracingTriangle::Draw()
{
    VulkanExampleBase::prepareFrame();
    traceRay->TraceRay();
    VulkanExampleBase::submitFrame();
}

void RaytracingTriangle::prepare()
{
    VulkanExampleBase::prepare();
    PrepareVertices();
    PrepareStorageBuffers();
    GeneratePrimaryRay();
    UpdateRayBuffers();
    PrepareRayTracing();
    PreparePipelines();
    UpdateUniformBuffers();
    buildCommandBuffers();
    prepared = true;
}

void RaytracingTriangle::render()
{
    if (!prepared || !swapChain.prepared)
        return;
    Draw();
    if (camera.updated) {
        UpdateUniformBuffers();
    }
}

void RaytracingTriangle::OnUpdateUIOverlay(vks::UIOverlay *overlay)
{
}

VULKAN_EXAMPLE_MAIN(RaytracingTriangle);