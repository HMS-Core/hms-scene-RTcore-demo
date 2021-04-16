/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan hybrid reflection demo head file.
 */

#ifndef VULKANEXAMPLES_HYBRIDREFLECTION_H
#define VULKANEXAMPLES_HYBRIDREFLECTION_H

#include <map>
#include "vulkanexamplebase.h"
#include "VulkanSkyboxPipeline.h"
#include "VulkanImageBasedLighting.h"
#include "VulkanPipelineFactory.h"
#include "VulkanTraceRay.h"
#include "examplebase/VulkanOffscreenRenderpass.h"

#include "BufferInfor.h"
#include "common.h"

#define ENABLE_VALIDATION false
#define HIT_BUFFER_DOWN_SCALE 1.0
#define MODEL_SCALE 2.0

class HybridReflection : public VulkanExampleBase {
public:
    HybridReflection();
    ~HybridReflection();

    void getEnabledFeatures() override;
    void loadAssets();
    void prepareStorageBuffers();
    void buildCommandBuffers() override;
    void updateMatrices();
    void upateParams();
    void updateUniformBuffers();
    void generateIBLTextures();
    void prepareOffscreenRenderpass();
    void preparePipelineResources();
    void prepareOnscreenPipelines();
    void prepareOffscreenPipelines();
    void prepareRayTracing();
    void draw();
    void prepare() override;
    virtual void render() override;
    virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay) override;

private:
    struct {
        RenderPipelinePtr sceneRender;
        RenderPipelinePtr skyboxRender;
        RenderPipelinePtr reflectionBlendRender;
        std::map<vkpip::RenderPipelineType, RenderPipelinePtr &> pipelines {
                {vkpip::SCENE_PBR_PIPELINE, sceneRender},
                {vkpip::SKY_BOX_PIPELINE, skyboxRender},
                {vkpip::REFLECTION_BLEND_PIPELINE, reflectionBlendRender}};
    } onScreenPipelines;

    struct {
        RenderPipelinePtr depthOnlyRender;
        RenderPipelinePtr generateRayRender;
        // post process of reflection
        RenderPipelinePtr reflectionRender;
        std::map<vkpip::RenderPipelineType, RenderPipelinePtr &> pipelines {
                {vkpip::DEPTH_ONLY_PIPELINE, depthOnlyRender},
                {vkpip::GENERATE_RAY_PIPELINE, generateRayRender},
                {vkpip::REFLECT_RENDER_PIPELINE, reflectionRender}};
    } offscreenPipelines;

    vkibl::VulkanImageBasedLighting *ibl = nullptr;

    std::unique_ptr<vkpass::VulkanOffscreenRenderpass> offscreenRenderpass = nullptr;
    // Ray tracing
    std::unique_ptr<rt::VulkanTraceRay> traceRay = nullptr;
    struct {
        vks::Buffer hitBuffer;
        vks::Buffer rayBuffer;
    } storageBuffers;

    struct {
        buf::UBOParams onScreen;
        buf::UBOParams offScreen;
    } uboParams;

    struct {
        buf::UBOMatrices skyBox;
        buf::UBOMatrices scene;
    } uboMatrices;

    vkpip::ExtraPipelineResources resources;

    uint32_t reflectionFramebufferHeight;
    uint32_t reflectionFramebufferWidth;
    struct Models {
        vkglTF::Model scene;
        vkglTF::Model sky;
    } models;
    vks::TextureCubeMap environmentCube;
    // direction light
    struct LightSource {
        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
    } lightSource;

    struct {
        // Synchronization fence to avoid rewriting compute CB if still in use
        VkFence generatRay;
        VkFence reflection;
    } fences;

    static constexpr float EXPOSURE_VAL = 4.5;
    static constexpr float GAMMA_VAL = 2.2;
};

#endif // VULKANEXAMPLES_HYBRIDREFLECTION_H

