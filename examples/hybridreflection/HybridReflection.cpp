/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan hybrid reflection demo.
 */

#include "HybridReflection.h"

HybridReflection::HybridReflection() : VulkanExampleBase(ENABLE_VALIDATION)
{
    title = "HybridReflection";

    camera.type = Camera::CameraType::lookat;
    camera.setPerspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
    camera.rotationSpeed = 0.25f;
    camera.movementSpeed = 0.1f;
    camera.setPosition({0.0f, 0.2f, 1.0f});
    camera.setRotation({0.0f, 0.f, 0.0f});

    settings.overlay = false;
}

HybridReflection::~HybridReflection()
{
    MY_DELETE(ibl);
    offscreenRenderpass = nullptr;
    traceRay = nullptr;
    environmentCube.destroy();
    vkDestroyFence(device, fences.generatRay, nullptr);
    vkDestroyFence(device, fences.reflection, nullptr);
    storageBuffers.hitBuffer.destroy();
    storageBuffers.rayBuffer.destroy();
}

void HybridReflection::getEnabledFeatures()
{
    if (deviceFeatures.fragmentStoresAndAtomics) {
        enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
    } else {
        vks::tools::exitFatal("Selected GPU does not support stores and atomic operations in the fragment stage",
                              VK_ERROR_FEATURE_NOT_PRESENT);
    }
};

void HybridReflection::loadAssets()
{
    // scene model's vertice and indices will be used in frament shader
    vkglTF::memoryPropertyFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    models.scene.loadFromFile(getAssetPath() + "models/H-rotation.gltf", vulkanDevice, queue, 0);
    // scky model doesn't need this flag
    vkglTF::memoryPropertyFlags = 0;
    models.sky.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, 0);
    environmentCube.loadFromFile(getAssetPath() + "enviroments/papermill.ktx", VK_FORMAT_R16G16B16A16_SFLOAT,
                                 vulkanDevice, queue);
};
void HybridReflection::prepareStorageBuffers()
{
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &storageBuffers.rayBuffer,
                               reflectionFramebufferWidth * reflectionFramebufferHeight * sizeof(RayShop::Ray));

    vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               &storageBuffers.hitBuffer,
                               reflectionFramebufferWidth * reflectionFramebufferHeight *
                                   RayShop::Traversal::GetHitFormatBytes(RayShop::TraceRayHitFormat::T_PRIMID_U_V));
}

void HybridReflection::buildCommandBuffers()
{
    // offcreen render pass
    {
        // For huawei logo scene we just process the node of ""huawei logo_obj"
        models.scene.setDrawMeshNames({"Huawei"});
        // Reflection Renderpass
        offscreenRenderpass->buildOffscreenRenderpass(&models.scene, &offscreenPipelines.pipelines);
        models.scene.setDrawMeshNames({"all"});
    }

    // On screen pass
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

            VkViewport viewport =
                vks::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
            vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

            VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
            vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

            onScreenPipelines.skyboxRender->draw(drawCmdBuffers[i], &models.sky);
            onScreenPipelines.sceneRender->draw(drawCmdBuffers[i], &models.scene);

            // For huawei logo scene we just process the node of ""huawei logo_obj"
            models.scene.setDrawMeshNames({"Huawei"});
            onScreenPipelines.reflectionBlendRender->draw(drawCmdBuffers[i], &models.scene);
            models.scene.setDrawMeshNames({"all"});
            drawUI(drawCmdBuffers[i]);

            vkCmdEndRenderPass(drawCmdBuffers[i]);

            VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
        }
    }
}

void HybridReflection::updateMatrices()
{
    // scene matrices
    uboMatrices.scene.projection = camera.matrices.perspective;
    uboMatrices.scene.view = camera.matrices.view;
    uboMatrices.scene.camPos =
        glm::vec3(-camera.position.z * sin(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)),
                  -camera.position.z * sin(glm::radians(camera.rotation.x)),
                  camera.position.z * cos(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)));
    // Center and scale model
    float scale = (1.0f / std::max(models.scene.dimensions.size[0],
                                   std::max(models.scene.dimensions.size[1], models.scene.dimensions.size[2]))) *
                  MODEL_SCALE;
    glm::vec3 translate = -glm::vec3(models.scene.dimensions.center[0], models.scene.dimensions.center[1],
                                     models.scene.dimensions.center[2]);

    uboMatrices.scene.model = glm::mat4(1.0f);
    uboMatrices.scene.model[0][0] = scale;
    uboMatrices.scene.model[1][1] = scale;
    uboMatrices.scene.model[2][2] = scale;
    uboMatrices.scene.model = glm::translate(uboMatrices.scene.model, translate);
    // skybox matrices
    uboMatrices.skyBox = uboMatrices.scene;
    uboMatrices.skyBox.model = glm::mat4(glm::mat3(camera.matrices.view));
}
void HybridReflection::upateParams()
{
    // on screen params
    uboParams.onScreen.lightDir =
        glm::vec4(sin(glm::radians(lightSource.rotation.x)) * cos(glm::radians(lightSource.rotation.y)),
                  sin(glm::radians(lightSource.rotation.y)),
                  cos(glm::radians(lightSource.rotation.x)) * cos(glm::radians(lightSource.rotation.y)), 0.0f);
    uboParams.onScreen.prefilteredCubeMipLevels = ibl->prefilteredCubeMipLevels;
    uboParams.onScreen.framebufferHeight = height;
    uboParams.onScreen.framebufferWidth = width;
    uboParams.onScreen.exposure = EXPOSURE_VAL;
    uboParams.onScreen.gamma = GAMMA_VAL;
    uboParams.offScreen = uboParams.onScreen;
    // Offscreen params
    uboParams.offScreen = uboParams.onScreen;
    uboParams.offScreen.framebufferWidth = reflectionFramebufferWidth;
    uboParams.offScreen.framebufferHeight = reflectionFramebufferHeight;
};
void HybridReflection::updateUniformBuffers()
{
    updateMatrices();
    upateParams();

    for (auto iter = onScreenPipelines.pipelines.begin(); iter != onScreenPipelines.pipelines.end(); iter++) {
        if (iter->first == vkpip::RenderPipelineType::SKY_BOX_PIPELINE) {
            iter->second->updateMatrices(&uboMatrices.skyBox);
        } else {
            iter->second->updateMatrices(&uboMatrices.scene);
        }

        iter->second->updateParams(&uboParams.onScreen);
    }

    for (auto iter = offscreenPipelines.pipelines.begin(); iter != offscreenPipelines.pipelines.end(); iter++) {
        iter->second->updateMatrices(&uboMatrices.scene);
        iter->second->updateParams(&uboParams.offScreen);
    }
}

void HybridReflection::generateIBLTextures()
{
    // Generate IBL textures
    ibl = new vkibl::VulkanImageBasedLighting(vulkanDevice, &models.sky, &environmentCube);
    ibl->generateIBLTextures(pipelineCache, queue);
}
void HybridReflection::prepareOffscreenRenderpass()
{
    offscreenRenderpass = std::make_unique<vkpass::VulkanOffscreenRenderpass>(vulkanDevice, reflectionFramebufferWidth,
                                                                              reflectionFramebufferHeight);
    offscreenRenderpass->prepareOffscreenPass();
}
void HybridReflection::preparePipelineResources()
{
    resources.textureCubeMap = &environmentCube;
    resources.ibl = ibl;
    resources.textureDescriptor = offscreenRenderpass->getReflectionTextureDescriptor();
    resources.indexBuffer = &models.scene.vertexIndexBufers.indices;
    resources.vertexBuffer = &models.scene.vertexIndexBufers.vertices;
    resources.rayBuffer = &storageBuffers.rayBuffer;
    resources.hitBuffer = &storageBuffers.hitBuffer;
}

void HybridReflection::prepareOnscreenPipelines()
{
    // Prepare skybox pipelines
    onScreenPipelines.skyboxRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(
        vkpip::SKY_BOX_PIPELINE, vulkanDevice, &resources, "hybridreflection/skybox.vert.spv",
        "hybridreflection/skybox.frag.spv");

    // Prepare PBR pipeline
    onScreenPipelines.sceneRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(
        vkpip::SCENE_PBR_PIPELINE, vulkanDevice, &resources, "hybridreflection/scene.vert.spv",
        "hybridreflection/pbr.frag.spv");

    // Prepare reflection blend on screen pipeline
    onScreenPipelines.reflectionBlendRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(
        vkpip::REFLECTION_BLEND_PIPELINE, vulkanDevice, &resources, "hybridreflection/scene.vert.spv",
        "hybridreflection/fullscreen.frag.spv");

    // pipeline layout:
    // set 0: 2 ubo, matrices and params (insert to the first palce of vector)
    // set 1: 3 ibl cube textures
    // set 2: 5 materials textures
    // set 3: 1 ubo, node matrices
    std::vector<VkDescriptorSetLayout> setLayouts = {vkibl::descriptorSetLayoutImage, vkglTF::descriptorSetLayoutImage,
                                                     vkglTF::descriptorSetLayoutUbo};
    std::vector<VkPushConstantRange> pushConstantRanges = {
        vks::initializers::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(buf::PushConstBlockMaterial), 0),
    };

    for (auto iter = onScreenPipelines.pipelines.begin(); iter != onScreenPipelines.pipelines.end(); iter++) {
        if (iter->first == vkpip::RenderPipelineType::SKY_BOX_PIPELINE) {
            iter->second->preparePipelines(renderPass, pipelineCache);
        } else {
            iter->second->preparePipelines(renderPass, pipelineCache, &setLayouts, &pushConstantRanges);
        }
    }
}

void HybridReflection::prepareOffscreenPipelines()
{
    offscreenPipelines.generateRayRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(
        vkpip::GENERATE_RAY_PIPELINE, vulkanDevice, &resources, "hybridreflection/scene.vert.spv",
        "hybridreflection/generateRay.frag.spv");
    // pipeline layout:
    // set 0: 2 ubo, matrices and params (insert to the first palce of vector)
    // set 1: 3 ibl cube textures
    // set 2: 5 materials textures
    // set 3: 1 ubo, node matrices
    std::vector<VkDescriptorSetLayout> setLayouts = {vkibl::descriptorSetLayoutImage, vkglTF::descriptorSetLayoutImage,
                                                     vkglTF::descriptorSetLayoutUbo};
    std::vector<VkPushConstantRange> pushConstantRanges = {
        vks::initializers::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(buf::PushConstBlockMaterial), 0),
    };

    offscreenPipelines.generateRayRender->preparePipelines(offscreenRenderpass->frameBuffers.generateRay->renderPass,
                                                           pipelineCache, &setLayouts, &pushConstantRanges);

    offscreenPipelines.depthOnlyRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(
        vkpip::DEPTH_ONLY_PIPELINE, vulkanDevice, &resources, "hybridreflection/scene.vert.spv", "");

    offscreenPipelines.depthOnlyRender->preparePipelines(offscreenRenderpass->frameBuffers.generateRay->renderPass,
                                                         pipelineCache, &setLayouts, &pushConstantRanges);

    offscreenPipelines.reflectionRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(
        vkpip::REFLECT_RENDER_PIPELINE, vulkanDevice, &resources, "hybridreflection/scene.vert.spv",
        "hybridreflection/reflection.frag.spv");
    offscreenPipelines.reflectionRender->preparePipelines(offscreenRenderpass->frameBuffers.reflection->renderPass,
                                                          pipelineCache, &setLayouts, &pushConstantRanges);

    // Fence for compute CB sync
    VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fences.generatRay));
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fences.reflection));
}

void HybridReflection::prepareRayTracing()
{
    // Trace ray pipeline
    traceRay = std::make_unique<rt::VulkanTraceRay>(vulkanDevice, reflectionFramebufferWidth,
                                                    reflectionFramebufferHeight, &storageBuffers.hitBuffer);
    traceRay->prepare();
    updateMatrices();
    traceRay->buildBVH(&models.scene, uboMatrices.scene.model);
}

void HybridReflection::draw()
{
    VulkanExampleBase::prepareFrame();

    // submmit generate ray commmand buffer
    // Use a fence to ensure that generateRay command buffer has finished executing
    // before using it again
    vkWaitForFences(device, 1, &fences.generatRay, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fences.generatRay);
    VkSubmitInfo offscreenSubmitInfo = vks::initializers::submitInfo();
    offscreenSubmitInfo.commandBufferCount = 1;
    offscreenSubmitInfo.pCommandBuffers = &offscreenRenderpass->commandBuffers.generateRay;
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &offscreenSubmitInfo, fences.generatRay));

    // submmit trace ray compute command buffer
    traceRay->traceRay(&storageBuffers.rayBuffer);

    // submmit reflection commmand buffer
    // Use a fence to ensure that reflection command buffer has finished executing
    // before using it again
    vkWaitForFences(device, 1, &fences.reflection, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fences.reflection);

    offscreenSubmitInfo.pCommandBuffers = &offscreenRenderpass->commandBuffers.reflection;
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &offscreenSubmitInfo, fences.reflection));

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VulkanExampleBase::submitFrame();
}

void HybridReflection::prepare()
{
    VulkanExampleBase::prepare();
    reflectionFramebufferHeight = uint32_t(height * HIT_BUFFER_DOWN_SCALE);
    reflectionFramebufferWidth = uint32_t(width * HIT_BUFFER_DOWN_SCALE);
    loadAssets();
    prepareStorageBuffers();
    generateIBLTextures();
    prepareRayTracing();
    prepareOffscreenRenderpass();
    preparePipelineResources();
    prepareOffscreenPipelines();
    prepareOnscreenPipelines();
    updateUniformBuffers();
    buildCommandBuffers();
    prepared = true;
}

void HybridReflection::render()
{
    if (!prepared || !swapChain.prepared) {
        return;
    }
    draw();
    if (camera.updated) {
        updateUniformBuffers();
    }
}

void HybridReflection::OnUpdateUIOverlay(vks::UIOverlay *overlay)
{
}

VULKAN_EXAMPLE_MAIN(HybridReflection);