/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan trace ray implementation file
 */

#include "VulkanTraceRay.h"
#include "Traversal.h"
#include "Log.h"
#include "RTATrace.h"
using namespace RayShop;
using namespace RayShop::Vulkan;

namespace rt {
constexpr const char *SUPPOR_SHADER_TYPE = ";frag;vert;comp;";

bool IsSupportShader(const std::string &posfix, const std::string &support)
{
    std::string str(";");
    str.append(posfix).append(";");
    return support.find(str) != std::string::npos;
}
std::string SplitFilename(const std::string &inputString)
{
    uint32_t found = inputString.find_last_of("/\\") + 1;
    std::string filename = inputString.substr(found, inputString.length() - found);
    return filename;
}

std::string GetFileSuffix(const std::string &inputString)
{
    uint32_t found = inputString.find_last_of('.');
    return inputString.substr(found + 1);
}

bool GetShaderStage(const std::string &shaderSuffix, ShaderStage &rayshopStage,
                    VkShaderStageFlagBits &shaderStage)
{
    const std::map<std::string, ShaderStage> RAYSHOP_SHADER_STAGE_MAP = {
        {"frag", ShaderStage::SHADER_STAGE_FRAGMENT_BIT},
        {"vert", ShaderStage::SHADER_STAGE_VERTEX_BIT},
        {"comp", ShaderStage::SHADER_STAGE_COMPUTE_BIT}};

    const std::map<std::string, VkShaderStageFlagBits> SHADER_STAGE_MAP = {{"frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                                                                           {"vert", VK_SHADER_STAGE_VERTEX_BIT},
                                                                           {"comp", VK_SHADER_STAGE_COMPUTE_BIT}};
    auto rayshopIter = RAYSHOP_SHADER_STAGE_MAP.find(shaderSuffix);
    if (rayshopIter == RAYSHOP_SHADER_STAGE_MAP.end()) {
        return false;
    }
    rayshopStage = rayshopIter->second;
    auto iter = SHADER_STAGE_MAP.find(shaderSuffix);
    if (iter == SHADER_STAGE_MAP.end()) {
        return false;
    }
    shaderStage = iter->second;
    return true;
}

std::string LoadShaderContent(const std::string &shaderFile)
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    std::string shaderContent = Utils::GetFileContent(getAssetPath() + shaderFile, androidApp->activity->assetManager);
#else
    std::string shaderContent = Utils::GetFileContent(getAssetPath() + shaderFile);
#endif
    return shaderContent;
}

VulkanTraceRay::~VulkanTraceRay() noexcept
{
    bvhBuffers.vertex.destroy();
    bvhBuffers.index.destroy();
    bvhBuffers.stagingBuffer.destroy();
    DestroyBVH();
    rayBuffer->destroy();
    hitBuffer->destroy();
    computeQueue = nullptr;
    gltfScene = nullptr;
    device = nullptr;
}

void VulkanTraceRay::Prepare()
{
    // Get a compute queue from the device
    vkGetDeviceQueue(device->logicalDevice, device->queueFamilyIndices.compute, 0, &computeQueue);
    traversal = std::make_unique<RayShop::Vulkan::Traversal>();
    // Using compute shader for raytracing
    traversal->Setup(device->physicalDevice, device->logicalDevice, computeQueue, device->queueFamilyIndices.compute);
    prepareFlag = true;
}

void VulkanTraceRay::ConvertLocalToWorld(std::vector<vkvert::Vertex> *vertices, const glm::mat4 &modelMatrix)
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

void VulkanTraceRay::BuildBVH(std::vector<uint32_t> *indices)
{
    ATRACE_CALL();
    buf::BVHMesh mesh;
    mesh.vertices = static_cast<float *>(static_cast<void *>(worldVertices.data()));
    mesh.numVertices = static_cast<uint32_t>(worldVertices.size());
    mesh.indices = static_cast<uint32_t*>(indices->data());
    mesh.numIndices = static_cast<uint32_t>(indices->size());
    mesh.stride = sizeof(vkvert::Vertex) / sizeof(float);

    size_t size = mesh.numVertices * sizeof(vkvert::Vertex);
    bvhBuffers.vertex.destroy();
    bvhBuffers.index.destroy();
    bvhBuffers.stagingBuffer.destroy();
#ifdef __ANDROID__
    device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bvhBuffers.vertex,
                         size, static_cast<void*>(mesh.vertices));
    VK_CHECK_RESULT(bvhBuffers.vertex.map());
#else
    device->createBufferWithStagigingBuffer(
        computeQueue, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &bvhBuffers.vertex, size, static_cast<void*>(mesh.vertices));
#endif
    device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &bvhBuffers.stagingBuffer, size);
    size = mesh.numIndices * sizeof(uint32_t);
    device->createBufferWithStagigingBuffer(
        computeQueue, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &bvhBuffers.index, size, static_cast<void *>(mesh.indices));

    RayShop::GeometryTriangleDescription geometry;
    geometry.stride = mesh.stride;
    geometry.vertices.cpuBuffer = mesh.vertices;
    geometry.vertices.type = RayShop::BufferType::CPU;
    geometry.verticesCount = mesh.numVertices;

    geometry.indices.cpuBuffer = mesh.indices;
    geometry.indices.type = RayShop::BufferType::CPU;
    geometry.indicesCount = mesh.numIndices;

    bvhGeometriesOnCPU.push_back(geometry);
    std::cout << "RTRender: indices num: " << mesh.numIndices << std::endl;
    blases.resize(bvhGeometriesOnCPU.size());
    traversal->CreateBLAS(RayShop::ASBuildMethod::SAH_CPU, bvhGeometriesOnCPU.size(), &bvhGeometriesOnCPU[0],
                          &blases[0]);

    std::vector<RayShop::InstanceDescription> intances;
    for (unsigned int blase : blases) {
        RayShop::InstanceDescription instance;
        instance.blas = blase;
        memset(instance.transform, 0, sizeof(instance.transform));
        instance.transform[0][0] = 1;
        instance.transform[1][1] = 1;
        instance.transform[2][2] = 1;
        instance.transform[3][3] = 1;
        intances.push_back(instance);
    }

    traversal->CreateTLAS(intances.size(), intances.data());
    std::cout << "RTRender: m_intances.size() = " << intances.size() << std::endl;
}

void VulkanTraceRay::UpdateBVH(const glm::mat4 &modelMatrix)
{
    ATRACE_CALL();
    gltfScene->convertLocalVertexToWorld(modelMatrix, worldVertices);
    {
        ATRACE_NAME("copyBVH");
        size_t copySize = worldVertices.size() * sizeof(vkvert::Vertex);
#ifdef __ANDROID__
        memcpy(bvhBuffers.vertex.mapped, worldVertices.data(), copySize);
#else
        VK_CHECK_RESULT(bvhBuffers.stagingBuffer.map());
        memcpy(bvhBuffers.stagingBuffer.mapped, worldVertices.data(), copySize);
        bvhBuffers.stagingBuffer.unmap();
        device->copyBuffer(&bvhBuffers.stagingBuffer, &bvhBuffers.vertex, computeQueue);
#endif
    }

    bvhGeometriesOnGPU.resize(bvhGeometriesOnCPU.size());
    for (size_t i = 0; i < bvhGeometriesOnGPU.size(); i++) {
        bvhGeometriesOnGPU[i].vertices.type = RayShop::BufferType::GPU;
        bvhGeometriesOnGPU[i].vertices.gpuVkBuffer = bvhBuffers.vertex.buffer;
        bvhGeometriesOnGPU[i].verticesCount = bvhGeometriesOnCPU[i].verticesCount;
        bvhGeometriesOnGPU[i].stride = bvhGeometriesOnCPU[i].stride;
        bvhGeometriesOnGPU[i].indices.type = RayShop::BufferType::GPU;
        bvhGeometriesOnGPU[i].indices.gpuVkBuffer = bvhBuffers.index.buffer;
        bvhGeometriesOnGPU[i].indicesCount = bvhGeometriesOnCPU[i].indicesCount;
    }
}

void VulkanTraceRay::RefitBVH(VkCommandBuffer cmd)
{
    traversal->RefitBLAS(bvhGeometriesOnGPU.size(), bvhGeometriesOnGPU.data(), blases.data(), cmd);
}

void VulkanTraceRay::DestroyBVH()
{
    traversal->DestroyBLAS(blases.size(), blases.data());
    traversal->Destroy();
}

void VulkanTraceRay::BuildBVH(std::vector<vkvert::Vertex> *vertices, std::vector<uint32_t> *indices,
                              const glm::mat4 &modelMatrix)
{
    ConvertLocalToWorld(vertices, modelMatrix);
    BuildBVH(indices);
    bvhBuildFlag = true;
}

void VulkanTraceRay::BuildBVH(vkglTF::Model *scene, const glm::mat4 &modelMatrix)
{
    ATRACE_CALL();
    gltfScene = scene;
    if (gltfScene->vertexBuffer.size() != worldVertices.size()) {
        worldVertices.clear();
        worldVertices.resize(gltfScene->vertexBuffer.size());
    }

    gltfScene->convertLocalVertexToWorld(modelMatrix, worldVertices);

    BuildBVH(&gltfScene->indexBuffer);
    bvhBuildFlag = true;
}

void VulkanTraceRay::TraceRay(VkCommandBuffer cmd)
{
    ASSERT(prepareFlag && bvhBuildFlag);
    ASSERT(hitBuffer && rayBuffer);
    RayShop::Buffer rayShopBuffer;
    rayShopBuffer.type = RayShop::BufferType::GPU;
    rayShopBuffer.gpuVkBuffer = rayBuffer->buffer;

    RayShop::Buffer rayShopHitBuffer;
    rayShopHitBuffer.type = RayShop::BufferType::GPU;
    rayShopHitBuffer.gpuVkBuffer = hitBuffer->buffer;
    RayShop::Result ret =
        traversal->TraceRays(rayCount, uint32_t(RayShop::TRACERAY_FLAG_INTERSECT_DEFAULT), rayShopBuffer,
                             rayShopHitBuffer, RayShop::TraceRayHitFormat::T_PRIMID_U_V, cmd);
    if (ret != RayShop::Result::SUCCESS) {
        std::cout << "RTRender trace error: " << (uint32_t)ret << std::endl;
    }
}

RayTracingDescriptors VulkanTraceRay::GetRaytracingDescriptors()
{
    RayShop::Result res =
        traversal->GetTraversalDescBufferInfos(&rayTracingDescriptors.bvhTree, &rayTracingDescriptors.bvhTriangles,
                                               &rayTracingDescriptors.tlasInfo, &rayTracingDescriptors.rtCoreUniforms);
    if (res != RayShop::Result::SUCCESS) {
        LOGE(" Failed to GetTraversalDescBufferInfos, err: %s", RayShop::Vulkan::Traversal::GetErrorCodeString(res));
        RayTracingDescriptors empty = {};
        return empty;
    }
    return rayTracingDescriptors;
}

void VulkanTraceRay::CreateRaytraceShaders(std::string shaderPath, std::vector<std::string> includeShadersKey,
                                           std::vector<std::string> includeShadersPath)
{
    ASSERT(includeShadersKey.size() == includeShadersPath.size());
    std::string shaderSuffix = GetFileSuffix(shaderPath);
    if (!IsSupportShader(shaderSuffix, SUPPOR_SHADER_TYPE)) {
        LOGE("Create shader Error: the input shader must be: %s", SUPPOR_SHADER_TYPE);
        return;
    }
    Vulkan::ShaderModule rtShaderModule;
    rtShaderModule.vkHandle = VK_NULL_HANDLE;
    Vulkan::RayTracingShaderModuleCreateInfo rtShaderCreateInfo = {};
    std::string shaderContent = LoadShaderContent(shaderPath);
    std::vector<const char *> shaderIncludes;
    std::vector<const char *> shaderIncludeSources;
    std::vector<std::string> commonContents;
    commonContents.resize(includeShadersKey.size());
    uint32_t includeIndex = 0;
    for (auto &includeShade : includeShadersPath) {
        shaderIncludes.push_back(includeShadersKey[includeIndex].c_str());
        commonContents[includeIndex] = LoadShaderContent(includeShade);
        shaderIncludeSources.push_back(commonContents[includeIndex].c_str());
        includeIndex++;
    }
    rtShaderCreateInfo.pName = "rtShader";
    rtShaderCreateInfo.pSource = shaderContent.c_str();
    Vulkan::ShaderStage rayshopStage;
    VkShaderStageFlagBits shaderStage;
    if (!GetShaderStage(shaderSuffix, rayshopStage, shaderStage)) {
        LOGE("Not support shader types!");
        return;
    }
    rtShaderCreateInfo.stage = rayshopStage;
    rtShaderCreateInfo.hitFormat = RayShop::TraceRayHitFormat::T_PRIMID_INSTID_U_V;
    rtShaderCreateInfo.ppShaderIncludes = shaderIncludes.data();
    rtShaderCreateInfo.shaderIncludesCnt = static_cast<uint32_t>(shaderIncludes.size());
    rtShaderCreateInfo.ppShaderIncludeSources = shaderIncludeSources.data();
    RayShop::Result res = traversal->CreateRayTracingShaderModule(&rtShaderCreateInfo, &rtShaderModule);
    if (res != RayShop::Result::SUCCESS) {
        LOGE("%s: failed to create raytracing shader module, err: %s.", __func__, traversal->GetErrorCodeString(res));
        return;
    }
    shaderModule = std::make_unique<Utils::ScopedShaderModule>(device->logicalDevice, rtShaderModule.vkHandle);
    if (!shaderModule->Valid()) {
        LOGE("%s: failed to create shader modules.", __func__);
        return;
    }
    shaderStageCreateInfo = Utils::PipelineShaderStageInfo(shaderModule->Get(), shaderStage, entryPoint);
}
} // namespace rt