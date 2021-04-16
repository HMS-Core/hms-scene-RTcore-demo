/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan trace ray implementation file
 */

#ifndef VULKANEXAMPLES_RAYTRACINGTRIANGLE_H
#define VULKANEXAMPLES_RAYTRACINGTRIANGLE_H
#include "vulkanexamplebase.h"
#include "VulkanVertex.h"
#include "BufferInfor.h"
#include "Traversal.h"
#include "VulkanTraceRay.h"
#include "VulkanTrianglePipeline.h"
#include "common.h"
#define ENABLE_VALIDATION false
#define MAXRAY_LENGTH 0xFFFF
#define HIT_BUFFER_DOWN_SCALE 0.5

class RaytracingTriangle : public VulkanExampleBase {
    struct ScreenCoordinates {
        glm::vec3 lookfrom;
        glm::vec3 start;
        glm::vec3 horizontal;
        glm::vec3 vertical;
    };

public:
    RaytracingTriangle() : VulkanExampleBase(ENABLE_VALIDATION)
    {
        title = "RayTracingTriangle";
        camera.type = Camera::CameraType::lookat;
        camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
        camera.rotationSpeed = 0.25f;
        camera.movementSpeed = 0.1f;
        camera.setPosition({0.0f, 0.f, 2.5f});
        camera.setRotation({0.0f, 0.f, 0.0f});
        settings.overlay = false;
        traceRayWidth = uint32_t(width * HIT_BUFFER_DOWN_SCALE);
        traceRayHeight = uint32_t(height * HIT_BUFFER_DOWN_SCALE);
        rayCount = traceRayHeight * traceRayWidth;
        rayDatas.resize(rayCount);
    };

    ~RaytracingTriangle();
    void updateCamPosition();
    void upateParams();
    void getScreenCoordinates(ScreenCoordinates &screen);
    void getPixelDir(const float u, const float v, const ScreenCoordinates &screen, RayShop::Ray &primaryRay);
    void updateRayBuffers();
    void generatePrimaryRay();
    void prepareVertices();
    void prepareStorageBuffers();
    void buildCommandBuffers() override;
    void updateUniformBuffers();
    void preparePipelines();
    void prepareRayTracing();
    void draw();
    void prepare() override;
    virtual void render() override;
    virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay) override;

private:
    vks::Buffer rayBuffer;
    vks::Buffer hitBuffer;
    vks::Buffer vertexBuffer;
    vks::Buffer indexBuffer;
    vks::Buffer stagingBuffer;

    std::vector<vkvert::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<RayShop::Ray> rayDatas;
    uint32_t rayCount = 0;
    // model / view / project matrices and camera positon
    buf::UBOParams uboParams;
    std::unique_ptr<vkpip::VulkanTrianglePipeline> triangleRender;
    ScreenCoordinates screenCoordinates;

    // Ray tracing
    std::unique_ptr<rt::VulkanTraceRay> traceRay;
    VkCommandBuffer copyCommand = VK_NULL_HANDLE;
    uint32_t traceRayHeight = 0;
    uint32_t traceRayWidth = 0;
    vkpip::ExtraPipelineResources resources;
};

#endif // VULKANEXAMPLES_RAYTRACINGTRIANGLE_H
