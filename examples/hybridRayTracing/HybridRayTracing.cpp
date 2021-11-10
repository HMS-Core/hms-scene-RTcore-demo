/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan Hybrid RayTracing implementation.
 */

#include "HybridRayTracing.h"

#include "VulkanSkyboxPipeline.h"
#include "VulkanOnscreenPipeline.h"

#include "RTATrace.h"
namespace rt {
namespace Detail {
constexpr float MODEL_SCALE = 2.0f;
constexpr float EXPOSURE_VAL = 4.5f;
constexpr float GAMMA_VAL = 2.2f;

#ifdef __ANDROID__
constexpr bool ENABLE_VALIDATION = false;
#else
constexpr bool ENABLE_VALIDATION = true;
#endif

void RegPipelines()
{
    vkpip::RegRenderPipeline<vkpip::VulkanScenePipeline>(vkpip::SCENE_PBR_PIPELINE);
    vkpip::RegRenderPipeline<vkpip::VulkanSkyboxPipeline>(vkpip::SKY_BOX_PIPELINE);
    vkpip::RegRenderPipeline<vkpip::VulkanOnscreenPipeline>(vkpip::REFLECTION_BLEND_PIPELINE);
}
} // namespace Detail

HybridRayTracing::HybridRayTracing() : VulkanExampleBase(Detail::ENABLE_VALIDATION)
{
    title = "HybridRayTracing";

    camera.type = Camera::CameraType::lookat;
    camera.setPerspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
    camera.rotationSpeed = 0.25f;
    camera.movementSpeed = 0.1f;
    camera.setPosition({ 0.0f, 0.f, 1.38f });
    camera.setRotation({ -9.0f, 0.5f, 0.0f });

    settings.overlay = true;
#ifdef __ANDROID__
    settings.vsync = true;
#endif

    m_loop = std::make_unique<EventLoop>();
    m_thread = std::thread { [this]() { m_loop->Loop(); } };
}

HybridRayTracing::~HybridRayTracing() noexcept
{
    m_environmentCube.destroy();
    m_loop->Quit();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void HybridRayTracing::LoadAssets()
{
    std::vector<std::string> fileNames = { "H-20w.gltf", "H-10w.gltf" }; //
    m_gltfModels = { "H-20w", "H-10w" };                                 //
    m_models.scene.resize(fileNames.size() * m_rtShaders.size());
    m_uboMatrices.scene.resize(fileNames.size() * m_rtShaders.size());
    for (size_t i = 0; i < fileNames.size(); i++) {
        for (size_t j = 0; j < m_rtShaders.size(); j++) {
            size_t idx = i * m_rtShaders.size() + j;
            m_models.scene[idx].loadFromFile(getAssetPath() + "models/" + fileNames[i], vulkanDevice, queue, 0);
        }
    }

    m_models.sky.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, 0);
    m_environmentCube.loadFromFile(getAssetPath() + "enviroments/papermill.ktx", VK_FORMAT_R16G16B16A16_SFLOAT,
        vulkanDevice, queue);
}

void HybridRayTracing::GenerateIBLTextures()
{
    // Generate IBL textures
    m_ibl = std::make_unique<vkibl::VulkanImageBasedLighting>(vulkanDevice, &m_models.sky, &m_environmentCube);
    m_ibl->GenerateIBLTextures(pipelineCache, queue);
}

void HybridRayTracing::PreparePipelineResources()
{
    m_resources.resize(m_gltfModels.size() * m_rtShaders.size());
    for (size_t i = 0; i != m_gltfModels.size(); ++i) {
        for (size_t j = 0; j != m_rtShaders.size(); ++j) {
            size_t idx = i * m_rtShaders.size() + j;
            m_resources[idx].textureDescriptor = &m_rayTracingPasses[idx]->GetColorAttachmentImageInfo();
            m_resources[idx].textureCubeMap = &m_environmentCube;
            m_resources[idx].ibl = m_ibl.get();
            m_resources[idx].indexBuffer = &m_models.scene[idx].vertexIndexBufers.indices;
            m_resources[idx].vertexBuffer = &m_models.scene[idx].vertexIndexBufers.vertices;
        }
    }
}

void HybridRayTracing::PrepareOnscreenPipelines()
{
    // Prepare skybox pipelines
    m_onScreenPipelines.skyboxRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(vkpip::SKY_BOX_PIPELINE,
        vulkanDevice, &m_resources[0], { "hybridRayTracing/skybox.vert.spv", "hybridRayTracing/skybox.frag.spv" });
    m_onScreenPipelines.skyboxRender->PreparePipelines(renderPass, pipelineCache, nullptr, nullptr,
                                                       0);

    // pipeline layout:
    // set 0: 2 ubo, matrices and params (insert to the first palce of vector)
    // set 1: 3 ibl cube textures
    // set 2: 5 materials textures
    // set 3: 1 ubo, node matrices
    std::vector<VkDescriptorSetLayout> setLayouts = { vkibl::descriptorSetLayoutImage, vkglTF::descriptorSetLayoutImage,
        vkglTF::descriptorSetLayoutUbo };
    std::vector<VkPushConstantRange> pushConstantRanges = {
        vks::initializers::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(buf::PushConstBlockMaterial), 0),
    };

    // Prepare PBR pipeline
    m_onScreenPipelines.sceneRender = vkpip::VulkanPipelineFactory::MakePipelineInstance(vkpip::SCENE_PBR_PIPELINE,
        vulkanDevice, &m_resources[0], { "hybridRayTracing/scene.vert.spv", "hybridRayTracing/pbr.frag.spv" });
    m_onScreenPipelines.sceneRender->PreparePipelines(renderPass, pipelineCache, &setLayouts,
                                                      &pushConstantRanges, 0);

    // Prepare reflection blend on screen pipelines
    m_onScreenPipelines.reflectionBlendRenders.resize(m_gltfModels.size() * m_rtShaders.size());
    for (size_t i = 0; i != m_gltfModels.size(); ++i) {
        for (size_t j = 0; j != m_rtShaders.size(); ++j) {
            int idx = i * m_rtShaders.size() + j;
            m_onScreenPipelines.reflectionBlendRenders[idx] =
                vkpip::VulkanPipelineFactory::MakePipelineInstance(vkpip::REFLECTION_BLEND_PIPELINE, vulkanDevice,
                &m_resources[idx], { "hybridRayTracing/scene.vert.spv", "hybridRayTracing/fullscreen.frag.spv" });
            m_onScreenPipelines.reflectionBlendRenders[idx]->PreparePipelines(renderPass,
                                                                              pipelineCache,
                                                                              &setLayouts,
                                                                              &pushConstantRanges,
                                                                              0);
        }
    }
}

bool HybridRayTracing::PrepareRayTracingPasses()
{
#if defined(__ANDROID__)
    m_downScale = 1080.0f / std::min(width, height);       // ray trace pass down scale to 1080.0f
    m_downScale = m_downScale > 1.0f ? 1.0f : m_downScale; // max down scale 1.0f
    m_downScale = m_downScale < 0.3f ? 0.3f : m_downScale; // min down scale 0.3f
    LOGI("%s: rayTrace pass w:%u, h:%u scale:%f", __func__, static_cast<uint32_t>(width * m_downScale),
        static_cast<uint32_t>(height * m_downScale), m_downScale);
#endif
    for (size_t i = 0; i != m_gltfModels.size(); ++i) {
        for (size_t j = 0; j != m_rtShaders.size(); ++j) {
            auto rtPass = std::make_unique<rt::RayTracingPass>(vulkanDevice, static_cast<uint32_t>(width * m_downScale),
                static_cast<uint32_t>(height * m_downScale));
            if (!rtPass->SetupRenderPass()) {
                return false;
            }
            if (!rtPass->InitTraversal()) {
                return false;
            }

            m_rayTracingPasses.push_back(std::move(rtPass));
        }
    }

    return true;
}

bool HybridRayTracing::PrepareRayTracingPipelines()
{
    for (size_t i = 0; i < m_gltfModels.size(); i++) {
        for (size_t j = 0; j < m_rtShaders.size(); j++) {
            size_t passId = i * m_rtShaders.size() + j;
            auto &rtPass = m_rayTracingPasses[passId];
            if (!rtPass->BuildBVH(m_models.scene[passId], m_uboMatrices.scene[passId].model)) {
                return false;
            }

            std::vector<VkDescriptorSetLayout> setLayouts = { vkibl::descriptorSetLayoutImage,
                vkglTF::descriptorSetLayoutImage, vkglTF::descriptorSetLayoutUbo };
            std::vector<VkPushConstantRange> pushConstantRanges = {
                vks::initializers::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(buf::PushConstBlockMaterial),
                    0),
            };
            if (!rtPass->SetupDepthOnlyPipeline(m_resources[passId], setLayouts, pushConstantRanges)) {
                return false;
            }
            if (!rtPass->SetupRayTracingPipeline(m_rtShaders[j], m_resources[passId], setLayouts,
                                                 pushConstantRanges)) {
                return false;
            }
        }
    }

    return true;
}

void HybridRayTracing::UpdateMatrices(int32_t index)
{
    m_uboMatrices.scene[index].projection = camera.matrices.perspective;
    m_uboMatrices.scene[index].view = camera.matrices.view;
    m_uboMatrices.scene[index].camPos =
        glm::vec3(-camera.position.z * sin(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)),
        -camera.position.z * sin(glm::radians(camera.rotation.x)),
        camera.position.z * cos(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)));
    // Center and scale model
    float scale = (1.0f / std::max(m_models.scene[index].dimensions.size[0],
        std::max(m_models.scene[index].dimensions.size[1], m_models.scene[index].dimensions.size[2]))) *
        Detail::MODEL_SCALE;
    glm::vec3 translate = -glm::vec3(m_models.scene[index].dimensions.center[0],
        m_models.scene[index].dimensions.center[1], m_models.scene[index].dimensions.center[2]);

    m_uboMatrices.scene[index].model = glm::mat4(1.0f);
    m_uboMatrices.scene[index].model[0][0] = scale;
    m_uboMatrices.scene[index].model[1][1] = scale;
    m_uboMatrices.scene[index].model[2][2] = scale;
    m_uboMatrices.scene[index].model = glm::translate(m_uboMatrices.scene[index].model, translate);
    // skybox matrices
    m_uboMatrices.skyBox = m_uboMatrices.scene[index];
    m_uboMatrices.skyBox.model = glm::mat4(glm::mat3(camera.matrices.view));
}

void HybridRayTracing::UpdateParams()
{
    // on screen params
    m_uboParams.onScreen.lightDir =
        glm::vec4(sin(glm::radians(m_lightSource.rotation.x)) * cos(glm::radians(m_lightSource.rotation.y)),
        sin(glm::radians(m_lightSource.rotation.y)),
        cos(glm::radians(m_lightSource.rotation.x)) * cos(glm::radians(m_lightSource.rotation.y)), 0.0f);
    m_uboParams.onScreen.prefilteredCubeMipLevels = m_ibl->prefilteredCubeMipLevels;
    m_uboParams.onScreen.framebufferHeight = height;
    m_uboParams.onScreen.framebufferWidth = width;
    m_uboParams.onScreen.exposure = Detail::EXPOSURE_VAL;
    m_uboParams.onScreen.gamma = Detail::GAMMA_VAL;

    // off screen params
    m_uboParams.offScreen.lightDir = m_uboParams.onScreen.lightDir;
    m_uboParams.offScreen.prefilteredCubeMipLevels = m_ibl->prefilteredCubeMipLevels;
    m_uboParams.offScreen.framebufferHeight = static_cast<uint32_t>(height * m_downScale);
    m_uboParams.offScreen.framebufferWidth = static_cast<uint32_t>(width * m_downScale);
    m_uboParams.offScreen.exposure = Detail::EXPOSURE_VAL;
    m_uboParams.offScreen.gamma = Detail::GAMMA_VAL;
}

void HybridRayTracing::UpdateUniformBuffers()
{
    int passId = m_models.index * m_rtShaders.size() + m_rtIndex;

    m_onScreenPipelines.skyboxRender->UpdateMatrices(&m_uboMatrices.skyBox);
    m_onScreenPipelines.skyboxRender->UpdateParams(&m_uboParams.onScreen);
    m_onScreenPipelines.sceneRender->UpdateMatrices(&m_uboMatrices.scene[passId]);
    m_onScreenPipelines.sceneRender->UpdateParams(&m_uboParams.onScreen);

    if (m_enableRT) {
        m_onScreenPipelines.reflectionBlendRenders[passId]->UpdateMatrices(&m_uboMatrices.scene[passId]);
        m_onScreenPipelines.reflectionBlendRenders[passId]->UpdateParams(&m_uboParams.onScreen);
        m_rayTracingPasses[passId]->UpdateMatrices(m_uboMatrices.scene[passId]);
        m_rayTracingPasses[passId]->UpdateParams(m_uboParams.offScreen);
    }
}

void HybridRayTracing::buildCommandBuffers()
{
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(cmdPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));

    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

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

        vkResetCommandBuffer(drawCmdBuffers[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        size_t passId = m_models.index * m_rtShaders.size() + m_rtIndex;

        if (m_enableRT) {
            m_rayTracingPasses[passId]->RefitBVH(drawCmdBuffers[i]);
            m_rayTracingPasses[passId]->Draw(drawCmdBuffers[i], m_ibl.get(), m_models.scene[passId]);
        }

        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport =
            vks::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        m_onScreenPipelines.skyboxRender->Draw(drawCmdBuffers[i], &m_models.sky);
        m_onScreenPipelines.sceneRender->Draw(drawCmdBuffers[i], &m_models.scene[passId]);

        if (m_enableRT) {
            m_models.scene[passId].setDrawMeshNames(m_reflectionMeshLists);
            m_onScreenPipelines.reflectionBlendRenders[passId]->Draw(drawCmdBuffers[i],
                                                                     &m_models.scene[passId]);
            m_models.scene[passId].setDrawMeshNames({"all" });
        }

        drawUI(drawCmdBuffers[i]);

        vkCmdEndRenderPass(drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}

void HybridRayTracing::prepare()
{
    VulkanExampleBase::prepare();
    Detail::RegPipelines();
    LoadAssets();
    GenerateIBLTextures();
    if (!PrepareRayTracingPasses()) {
        return;
    }
    PreparePipelineResources();
    PrepareOnscreenPipelines();
    // update Matrices for every scene/pass !
    for (size_t i = 0; i != m_gltfModels.size() * m_rtShaders.size(); ++i) {
        UpdateMatrices(static_cast<uint32_t>(i));
    }
    UpdateParams();
    UpdateUniformBuffers();
    if (!PrepareRayTracingPipelines()) {
        return;
    }
    buildCommandBuffers();
    prepared = true;
}

void HybridRayTracing::UpdateResourceAsyncly()
{
    m_loop->RunInLoop([this]() {
        ATRACE_NAME("update resources");
        {
            ATRACE_NAME("waiting for last frame done.");
            std::unique_lock<std::mutex> lock(m_mutex);
            m_updateCond.wait(lock, [this]() { return m_readyToUpdate; });
            m_readyToUpdate = false;
        }

        if (!paused) {
            size_t passId = m_models.index * m_rtShaders.size() + m_rtIndex;
            if (m_enableAnimate && (!m_models.scene[passId].animations.empty())) {
                m_animationTimer += frameTimer;
                if (m_animationTimer > m_models.scene[passId].animations[0].end) {
                    m_animationTimer -= m_models.scene[passId].animations[0].end;
                }
                m_models.scene[passId].updateAnimation(0, m_animationTimer);
            }
            if (m_enableRT) {
                m_rayTracingPasses[passId]->UpdateBVH(m_models.scene[passId], m_uboMatrices.scene[passId].model);
            }

            if (camera.updated) {
                for (size_t i = 0; i != m_gltfModels.size() * m_rtShaders.size(); ++i) {
                    UpdateMatrices(static_cast<uint32_t>(i));
                }
                UpdateParams();
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_readyToDraw = true;
            m_drawCond.notify_one();
        }
    });
}

void HybridRayTracing::render()
{
    if (!prepared || !swapChain.prepared) {
        return;
    }

    UpdateResourceAsyncly();

    ATRACE_NAME("main Draw");
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_readyToUpdate = true;
        m_updateCond.notify_one();
    }

    prepareFrame();

    if (camera.updated) {
        UpdateUniformBuffers();
    }

    {
        ATRACE_NAME("waiting for resources updated.");
        std::unique_lock<std::mutex> lock(m_mutex);
        m_drawCond.wait(lock, [this]() { return m_readyToDraw; });
        m_readyToDraw = false;
    }

    submitFrame(m_addWait);
}

void HybridRayTracing::OnUpdateUIOverlay(vks::UIOverlay *overlay)
{
    if (overlay->header("Setting")) {
        bool reBuild = false;
        reBuild |= (overlay->comboBox("Models", &m_models.index, m_gltfModels));
        // if enable shader choice. overlay->comboBox("rtShaders", &m_rtIndex, m_rtShaders);
        reBuild |= (overlay->checkBox("RayTrace", &m_enableRT));

        overlay->checkBox("Animate", &m_enableAnimate);
        if (overlay->checkBox("Stat", &m_showStat)) {
            m_rtIndex = m_showStat ? 1 : 0;
            reBuild = true;
        }
        if (reBuild) {
            auto passId = m_models.index * m_rtShaders.size() + m_rtIndex;
            m_rayTracingPasses[passId]->SetStat(m_showStat);
            UpdateUniformBuffers();
            buildCommandBuffers();
        }
    }
    if (m_showStat && m_reflectArea > 1e-4) {
        ImGui::Text("RT Reflect Area: %.2f%%", m_reflectArea * 100.0f);
    }
}

void HybridRayTracing::OnNextFrame()
{
    if (!m_enableRT || !m_showStat) {
        m_reflectArea = 0.0f;
        return;
    }
    auto passId = m_models.index * m_rtShaders.size() + m_rtIndex;
    m_reflectArea = m_rayTracingPasses[passId]->GetReflectArea();
}
} // namespace rt

VULKAN_EXAMPLE_MAIN(rt::HybridRayTracing);
