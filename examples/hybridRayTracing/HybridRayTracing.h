/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan Hybrid RayTracing declaration.
 */

#ifndef VULKANEXAMPLES_HYBRIDRAYTRACING_H
#define VULKANEXAMPLES_HYBRIDRAYTRACING_H

#include "BufferInfor.h"
#include "RayTracingPass.h"
#include "SaschaWillemsVulkan/vulkanexamplebase.h"
#include "VulkanPipelineFactory.h"
#include "EventLoop.h"

#include <thread>

namespace rt {
class HybridRayTracing : public VulkanExampleBase {
public:
    HybridRayTracing();
    ~HybridRayTracing() noexcept override;

    void render() override;
    void prepare() override;
    void OnUpdateUIOverlay(vks::UIOverlay *overlay) override;
    void buildCommandBuffers() override;
    void OnNextFrame() override;

private:
    void GenerateIBLTextures();
    void LoadAssets();
    void PreparePipelineResources();
    void PrepareOnscreenPipelines();
    bool PrepareRayTracingPasses();
    bool PrepareRayTracingPipelines();
    void UpdateMatrices(int32_t index);
    void UpdateParams();
    void UpdateUniformBuffers();

    void UpdateResourceAsyncly();

    float m_animationTimer = 0.0f;

    struct LightSource {
        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
    } m_lightSource;

    struct {
        buf::UBOMatrices skyBox;
        std::vector<buf::UBOMatrices> scene;
    } m_uboMatrices;

    struct {
        buf::UBOParams onScreen;
        buf::UBOParams offScreen;
    } m_uboParams;

    std::vector<std::string> m_gltfModels;
    struct Models {
        std::vector<vkglTF::Model> scene;
        vkglTF::Model sky;
        int32_t index = 0;
    } m_models;
    std::unordered_set<std::string> m_reflectionMeshLists = {"all"};

    vks::TextureCubeMap m_environmentCube;

    std::unique_ptr<vkibl::VulkanImageBasedLighting> m_ibl;

    struct {
        RenderPipelinePtr sceneRender;
        RenderPipelinePtr skyboxRender;
        std::vector<RenderPipelinePtr> reflectionBlendRenders;
    } m_onScreenPipelines;
    std::vector<vkpip::ExtraPipelineResources> m_resources;       // n_model x n_rtShader

    int32_t m_rtIndex = 0;
    // multi-rt shaders base: "raytracing_white.frag"
    std::vector<std::string> m_rtShaders = {"raytracing_color.frag", "raytracing_stats.frag"};
    std::vector<std::unique_ptr<rt::RayTracingPass>> m_rayTracingPasses;        // n_model x n_rtShader
    bool m_enableRT = true;
    float m_downScale = 1.0f;
    bool m_showStat = false;
    float m_reflectArea = 0.0f;

    std::unique_ptr<EventLoop> m_loop;
    std::thread m_thread;
    bool m_readyToDraw = false;
    bool m_readyToUpdate = false;
    std::condition_variable m_drawCond;
    std::condition_variable m_updateCond;
    mutable std::mutex m_mutex;
    useconds_t m_addWait = 3000;        // additional waiting, hack for frame tear on snapdragon 888 chip.
};
} // namespace rt
#endif // VULKANEXAMPLES_HYBRIDRAYTRACING_H
