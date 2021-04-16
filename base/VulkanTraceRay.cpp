/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan trace ray implementation file
 */

#include "VulkanTraceRay.h"
#include "Traversal.h"

namespace rt {
void VulkanTraceRay::prepare()
{
    // Get a compute queue from the device
    vkGetDeviceQueue(device->logicalDevice, device->queueFamilyIndices.compute, 0, &computeQueue);
    // Using compute shader for raytracing
    traversal.Setup(device->physicalDevice, device->logicalDevice, computeQueue, device->queueFamilyIndices.compute);
}

void VulkanTraceRay::convertLocalToWorld(std::vector<vkvert::Vertex> *vertices, const glm::mat4 &modelMatrix)
{
    if (vertices->size() != worldVertices.size()) {
        worldVertices.clear();
        worldVertices.resize(vertices->size());
    }
    uint32_t vertexIndex = 0;
    for (auto &vertex : *vertices) {
        vkvert::Vertex worldVertex = vertex;
        worldVertex.pos = modelMatrix * glm::vec4(glm::vec3(vertex.pos), 1.0f);
        // Flip Y-Axis of vertex positions
        worldVertex.pos.y *= -1.0f;
        worldVertex.pos = glm::vec4(worldVertex.pos.x / worldVertex.pos.w, worldVertex.pos.y / worldVertex.pos.w,
                                    worldVertex.pos.z / worldVertex.pos.w, 1.f);
        worldVertices[vertexIndex] = worldVertex;
        vertexIndex++;
    }
}

void VulkanTraceRay::buildBVH(std::vector<uint32_t> *indices)
{
    buf::BVHMesh mesh;
    mesh.vertices = (float *)worldVertices.data();
    mesh.numVertices = static_cast<uint32_t>(worldVertices.size());
    mesh.indices = (uint32_t *)indices->data();
    mesh.numIndices = static_cast<uint32_t>(indices->size());
    mesh.stride = sizeof(vkvert::Vertex) / sizeof(float);

    RayShop::GeometryTriangleDescription geometry;
    geometry.stride = mesh.stride;
    geometry.vertice.cpuBuffer = mesh.vertices;
    geometry.vertice.type = RayShop::BufferType::CPU;
    geometry.verticeCount = mesh.numVertices;

    geometry.indice.cpuBuffer = mesh.indices;
    geometry.indice.type = RayShop::BufferType::CPU;
    geometry.indiceCount = mesh.numIndices;
    std::vector<RayShop::GeometryTriangleDescription> geometries;
    geometries.push_back(geometry);
    std::cout << "RTRender: indice num: " << mesh.numIndices << std::endl;
    std::vector<RayShop::BLAS> blas;
    blas.resize(geometries.size());
    RayShop::Result ret;

    ret = traversal.CreateBLAS(RayShop::ASBuildMethod::SAH_CPU, geometries.size(), &geometries[0], &blas[0]);
    std::vector<RayShop::InstanceDescription> intances;
    for (int i = 0; i < blas.size(); i++) {
        RayShop::InstanceDescription instance;
        instance.blas = blas[i];
        memset(instance.transform, 0, sizeof(instance.transform));
        instance.transform[0][0] = 1;
        instance.transform[1][1] = 1;
        instance.transform[2][2] = 1;
        instance.transform[3][3] = 1;
        intances.push_back(instance);
    }

    ret = traversal.CreateTLAS(intances.size(), intances.data());
    std::cout << "RTRender: CreateTLAS ret: " << uint32_t(ret) << ", m_intances.size() = " << intances.size()
              << std::endl;
}

void VulkanTraceRay::buildBVH(std::vector<vkvert::Vertex> *vertices, std::vector<uint32_t> *indices,
                              const glm::mat4 &modelMatrix)
{
    convertLocalToWorld(vertices, modelMatrix);
    buildBVH(indices);
}

void VulkanTraceRay::buildBVH(vkglTF::Model *scene, const glm::mat4 &modelMatrix)
{
    if (scene->vertexBuffer.size() != worldVertices.size()) {
        worldVertices.clear();
        worldVertices.resize(scene->vertexBuffer.size());
    }

    scene->convertLocalVertexToWorld(modelMatrix, worldVertices);

    buildBVH(&scene->indexBuffer);
}

void VulkanTraceRay::traceRay(vks::Buffer *rayBuffer)
{
    RayShop::Buffer rayShopBuffer;
    rayShopBuffer.type = RayShop::BufferType::GPU;
    rayShopBuffer.gpuVkBuffer = rayBuffer->buffer;

    RayShop::Buffer rayShopHitBuffer;
    rayShopHitBuffer.type = RayShop::BufferType::GPU;
    rayShopHitBuffer.gpuVkBuffer = hitBuffer->buffer;
    RayShop::Result ret =
        traversal.TraceRays(rayCount, uint32_t(RayShop::TRACERAY_FLAG_INTERSECT_DEFAULT), rayShopBuffer,
                            rayShopHitBuffer, RayShop::TraceRayHitFormat::T_PRIMID_U_V);
    if (ret != RayShop::Result::SUCCESS) {
        std::cout << "RTRender trace error: " << (uint32_t)ret << std::endl;
    }
}
} // namespace rt