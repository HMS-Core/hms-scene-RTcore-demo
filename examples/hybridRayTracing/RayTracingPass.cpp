/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Vulkan RayTracing RenderPass implementation.
 */

#include "RayTracingPass.h"
#include "SaschaWillemsVulkan/VulkanInitializers.hpp"
#include "Utils.h"
#include "Log.h"

#ifdef __ANDROID__
#include <jni.h>
static JavaVM *g_JVM = nullptr;
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    (void)reserved;
    g_JVM = vm;
    return JNI_VERSION_1_6;
}
JNIEnv *CreateJNIEnv()
{
    if (g_JVM == nullptr) {
        return nullptr;
    }
    JNIEnv *jniEnv = nullptr;
    bool detached = g_JVM->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6) == JNI_EDETACHED;
    if (detached) {
        g_JVM->AttachCurrentThread(&jniEnv, nullptr);
    }
    return jniEnv;
}
#endif

namespace rt {
namespace detail {
class RTDescSet : public DescSet {
public:
    explicit RTDescSet(vks::VulkanDevice *vulkandevice) : DescSet(vulkandevice) {}
    ~RTDescSet() noexcept override = default;

private:
    bool InitLayout() override
    {
        ASSERT(m_vulkanDevice != nullptr);
        ASSERT(m_vulkanDevice->logicalDevice != VK_NULL_HANDLE);

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 5),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 6),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 7),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT, 8),
        };

        VkDescriptorSetLayoutCreateInfo descLayoutCreateInfo {};
        descLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descLayoutCreateInfo.pBindings = setLayoutBindings.data();
        descLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

        VkResult res = vkCreateDescriptorSetLayout(m_vulkanDevice->logicalDevice, &descLayoutCreateInfo, nullptr,
            &m_descSetLayout);
        if (res != VK_SUCCESS) {
            m_descSetLayout = VK_NULL_HANDLE;
            LOGE("%s: Failed to create RT DescSetLayout, err: %d.", __func__, static_cast<int>(res));
            return false;
        }

        return true;
    }
};

class DepthOnlyDescSet : public DescSet {
public:
    explicit DepthOnlyDescSet(vks::VulkanDevice *vulkandevice) : DescSet(vulkandevice) {}
    ~DepthOnlyDescSet() noexcept override = default;

private:
    bool InitLayout() override
    {
        ASSERT(m_vulkanDevice != nullptr);
        ASSERT(m_vulkanDevice->logicalDevice != VK_NULL_HANDLE);

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                0),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                1)
        };

        VkDescriptorSetLayoutCreateInfo descLayoutCreateInfo {};
        descLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descLayoutCreateInfo.pBindings = setLayoutBindings.data();
        descLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

        VkResult res = vkCreateDescriptorSetLayout(m_vulkanDevice->logicalDevice, &descLayoutCreateInfo, nullptr,
            &m_descSetLayout);
        if (res != VK_SUCCESS) {
            m_descSetLayout = VK_NULL_HANDLE;
            LOGE("%s: Failed to create DepthOnly DescSetLayout, err: %d.", __func__, static_cast<int>(res));
            return false;
        }

        return true;
    }
};
} // namespace detail

RayTracingPass::RayTracingPass(vks::VulkanDevice *vulkandevice, uint32_t width, uint32_t height)
    : m_vulkandevice(vulkandevice), m_width(width), m_height(height)
{}

RayTracingPass::~RayTracingPass() noexcept
{
    m_bvhStagingBuffers.vertex.destroy();
    m_bvhStagingBuffers.index.destroy();
    m_bvhBuffers.vertex.destroy();
    m_bvhBuffers.index.destroy();
    m_countBuffer.destroy();
    m_countStagingBuffer.destroy();

    m_uniformBuffers.matrices.destroy();
    m_uniformBuffers.params.destroy();

    m_traversal->DestroyBLAS(static_cast<uint32_t>(m_blases.size()), m_blases.data());
    m_traversal->Destroy();

    m_vulkandevice = nullptr;
}

bool RayTracingPass::InitTraversal()
{
    m_traversal = std::make_unique<RayShop::Vulkan::Traversal>();

    VkQueue computeQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_vulkandevice->logicalDevice, m_vulkandevice->queueFamilyIndices.compute, 0, &computeQueue);

#ifdef __ANDROID__
    RayShop::Result res = m_traversal->Setup(m_vulkandevice->physicalDevice, m_vulkandevice->logicalDevice,
        computeQueue, m_vulkandevice->queueFamilyIndices.compute, CreateJNIEnv());
#else
    RayShop::Result res = m_traversal->Setup(m_vulkandevice->physicalDevice, m_vulkandevice->logicalDevice,
        computeQueue, m_vulkandevice->queueFamilyIndices.compute);
#endif
    if (res != RayShop::Result::SUCCESS) {
        LOGE("Failed to setup traversal, err: %s.", m_traversal->GetErrorCodeString(res));
        return false;
    }

    return true;
}

bool RayTracingPass::BuildBVH(vkglTF::Model &scene, const glm::mat4 &modelMatrix)
{
    m_worldVertices.resize(scene.vertexBuffer.size());
    scene.convertLocalVertexToWorld(modelMatrix, m_worldVertices);

    size_t size = m_worldVertices.size() * sizeof(vkvert::Vertex);
    m_vulkandevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_bvhBuffers.vertex, size);
    m_bvhBuffers.vertex.setupDescriptor();
    m_vulkandevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_bvhStagingBuffers.vertex, size,
        m_worldVertices.data());
    VK_CHECK_RESULT(m_bvhStagingBuffers.vertex.map());

    size = scene.indexBuffer.size() * sizeof(uint32_t);
    m_vulkandevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_bvhBuffers.index, size);
    m_bvhBuffers.index.setupDescriptor();
    m_vulkandevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &m_bvhStagingBuffers.index, size,
        scene.indexBuffer.data());
    VK_CHECK_RESULT(m_bvhStagingBuffers.index.map());

    uint32_t zeros[m_countSize]{0};
    m_vulkandevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_countBuffer, m_countSize);
    m_countBuffer.setupDescriptor();
    m_vulkandevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &m_countStagingBuffer, m_countSize, zeros);
    VK_CHECK_RESULT(m_countStagingBuffer.map());

    RayShop::GeometryTriangleDescription geometry;
    geometry.stride = sizeof(vkvert::Vertex) / sizeof(float);
    geometry.vertices.cpuBuffer = m_worldVertices.data();
    geometry.vertices.type = RayShop::BufferType::CPU;
    geometry.verticesCount = static_cast<uint32_t>(m_worldVertices.size());
    geometry.indices.cpuBuffer = scene.indexBuffer.data();
    geometry.indices.type = RayShop::BufferType::CPU;
    geometry.indicesCount = static_cast<uint32_t>(scene.indexBuffer.size());

    m_bvhGeometriesOnCPU.push_back(geometry);
    m_blases.resize(m_bvhGeometriesOnCPU.size());
    RayShop::Result res = m_traversal->CreateBLAS(RayShop::ASBuildMethod::SAH_CPU,
        static_cast<uint32_t>(m_bvhGeometriesOnCPU.size()), m_bvhGeometriesOnCPU.data(), m_blases.data());
    if (res != RayShop::Result::SUCCESS) {
        LOGE("Failed to CreateBLAS, err: %s.", m_traversal->GetErrorCodeString(res));
        return false;
    }

    std::vector<RayShop::InstanceDescription> intances;
    for (unsigned int m_blase : m_blases) {
        RayShop::InstanceDescription instance {};
        instance.blas = m_blase;
        instance.transform[0][0] = 1;
        instance.transform[1][1] = 1;
        instance.transform[2][2] = 1;
        instance.transform[3][3] = 1;
        intances.push_back(instance);
    }
    res = m_traversal->CreateTLAS(static_cast<uint32_t>(intances.size()), intances.data());
    if (res != RayShop::Result::SUCCESS) {
        LOGE("Failed to CreateTLAS, err: %s.", m_traversal->GetErrorCodeString(res));
        return false;
    }

    m_bvhGeometriesOnGPU.resize(m_bvhGeometriesOnCPU.size());
    for (size_t i = 0; i < m_bvhGeometriesOnGPU.size(); i++) {
        m_bvhGeometriesOnGPU[i].vertices.type = RayShop::BufferType::GPU;
        m_bvhGeometriesOnGPU[i].vertices.gpuVkBuffer = m_bvhBuffers.vertex.buffer;
        m_bvhGeometriesOnGPU[i].verticesCount = m_bvhGeometriesOnCPU[i].verticesCount;
        m_bvhGeometriesOnGPU[i].stride = m_bvhGeometriesOnCPU[i].stride;
        m_bvhGeometriesOnGPU[i].indices.type = RayShop::BufferType::GPU;
        m_bvhGeometriesOnGPU[i].indices.gpuVkBuffer = m_bvhBuffers.index.buffer;
        m_bvhGeometriesOnGPU[i].indicesCount = m_bvhGeometriesOnCPU[i].indicesCount;
    }

    return true;
}

void RayTracingPass::UpdateBVH(vkglTF::Model &scene, const glm::mat4 &modelMatrix)
{
    scene.convertLocalVertexToWorld(modelMatrix, m_worldVertices);

    size_t size = m_worldVertices.size() * sizeof(vkvert::Vertex);
    memcpy(m_bvhStagingBuffers.vertex.mapped, m_worldVertices.data(), size);
    size = scene.indexBuffer.size() * sizeof(uint32_t);
    memcpy(m_bvhStagingBuffers.index.mapped, scene.indexBuffer.data(), size);
}

void RayTracingPass::RefitBVH(VkCommandBuffer cmdBuffer)
{
    RayShop::Result res = m_traversal->RefitBLAS(static_cast<uint32_t>(m_bvhGeometriesOnGPU.size()),
        m_bvhGeometriesOnGPU.data(), m_blases.data(), cmdBuffer);
    if (res != RayShop::Result::SUCCESS) {
        LOGE("Failed to RefitBLAS, err: %s.", m_traversal->GetErrorCodeString(res));
    }

    if (cmdBuffer != VK_NULL_HANDLE) {
        Utils::BarrierInfo barrierInfo {};
        barrierInfo.srcMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrierInfo.dstMask = VK_ACCESS_MEMORY_READ_BIT;
        barrierInfo.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrierInfo.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        Utils::SetMemoryBarrier(cmdBuffer, barrierInfo);
    }
}

bool RayTracingPass::SetupRenderPass()
{
    m_framebuffer = std::make_unique<vks::Framebuffer>(m_vulkandevice);
    m_framebuffer->width = m_width;
    m_framebuffer->height = m_height;

    // Two attachments (1 color, 1 depth)
    // Color attachment
    vks::AttachmentCreateInfo attachmentInfo {};
    attachmentInfo.width = m_width;
    attachmentInfo.height = m_height;
    attachmentInfo.layerCount = 1;
    attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_framebuffer->addAttachment(attachmentInfo);

    // Depth attachment
    // Find a suitable depth format
    VkFormat attDepthFormat = VK_FORMAT_UNDEFINED;
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(m_vulkandevice->physicalDevice, &attDepthFormat);
    if (validDepthFormat == VK_FALSE) {
        LOGE("Can't find suitable depth format.");
        return false;
    }

    attachmentInfo.format = attDepthFormat;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_framebuffer->addAttachment(attachmentInfo);

    // Create sampler to sample from the color attachments
    VkResult res =
        m_framebuffer->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    if (res != VK_SUCCESS) {
        LOGE("%s: Failed to createSampler, err: %d.", __func__, static_cast<int>(res));
        return false;
    }

    // Create default renderpass for the framebuffer
    res = m_framebuffer->createRenderPass(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    if (res != VK_SUCCESS) {
        LOGE("%s: Failed to CreateRenderPass, err: %d.", __func__, static_cast<int>(res));
        return false;
    }

    m_reflTexDescInfo = vks::initializers::descriptorImageInfo(m_framebuffer->sampler,
        m_framebuffer->attachments[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    SetupUniformBuffers();

    return true;
}

bool RayTracingPass::CreateRayTracingDescSet()
{
    m_rtDescSet = std::make_unique<detail::RTDescSet>(m_vulkandevice);
    if (!m_rtDescSet->Init()) {
        LOGE("%s: Failed to create raytracing descset.", __func__);
        return false;
    }

    return true;
}

bool RayTracingPass::SetupDepthOnlyPipeline(const vkpip::ExtraPipelineResources &resource,
    const std::vector<VkDescriptorSetLayout> &setLayouts, const std::vector<VkPushConstantRange> &pushConstantRanges,
    VkPipelineCache pipelineCache)
{
    m_depthOnlyDescSet = std::make_unique<detail::DepthOnlyDescSet>(m_vulkandevice);
    if (!m_depthOnlyDescSet->Init()) {
        LOGE("%s: Failed to create depth only descset.", __func__);
        return false;
    }

    m_depthOnlyPipeline = std::make_unique<GraphicPipeline>(m_vulkandevice);

    std::vector<VkDescriptorSetLayout> depthSetLayouts;
    depthSetLayouts.push_back(m_depthOnlyDescSet->GetDescSetLayout());
    for (auto layout : setLayouts) {
        depthSetLayouts.push_back(layout);
    }

    VkDevice device = m_vulkandevice->logicalDevice;
    std::string vertShaderSpvFile = getAssetPath() + "shaders/glsl/hybridRayTracing/scene.vert.spv";
    Utils::ScopedShaderModule vertShaderModule(device,
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        vks::tools::loadShader(androidApp->activity->assetManager, vertShaderSpvFile.c_str(), device));
#else
        vks::tools::loadShader(vertShaderSpvFile.c_str(), device));
#endif

    std::string entryPoint = "main";
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { Utils::PipelineShaderStageInfo(vertShaderModule.Get(),
        VK_SHADER_STAGE_VERTEX_BIT, entryPoint) };

    std::vector<vkvert::VertexComponent> components = { vkvert::VertexComponent::Position,
        vkvert::VertexComponent::Normal, vkvert::VertexComponent::UV, vkvert::VertexComponent::Joint0,
        vkvert::VertexComponent::Weight0 };
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
            vkvert::Vertex::InputBindingDescription(0) };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes =
            vkvert::Vertex::InputAttributeDescriptions(0, components);
    m_depthOnlyPipeline->SetVertexInputBindings(vertexInputBindings);
    m_depthOnlyPipeline->SetVertexInputAttributes(vertexInputAttributes);
    if (!m_depthOnlyPipeline->Setup(depthSetLayouts, pushConstantRanges, m_framebuffer->renderPass, shaderStages,
        pipelineCache)) {
        return false;
    }

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = { Utils::WriteDescriptorSet(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, m_uniformBuffers.matrices.descriptor),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_uniformBuffers.params.descriptor) };

    m_depthOnlyDescSet->Update(writeDescriptorSets);

    return true;
}

bool RayTracingPass::SetupRayTracingPipeline(const std::string& shaderName,
    const vkpip::ExtraPipelineResources &resource, const std::vector<VkDescriptorSetLayout> &setLayouts,
    const std::vector<VkPushConstantRange> &pushConstantRanges, VkPipelineCache pipelineCache)
{
    if (!CreateRayTracingDescSet()) {
        return false;
    }

    m_rtPipeline = std::make_unique<GraphicPipeline>(m_vulkandevice);

    std::vector<VkDescriptorSetLayout> rtSetLayouts;
    rtSetLayouts.push_back(m_rtDescSet->GetDescSetLayout());
    for (auto layout : setLayouts) {
        rtSetLayouts.push_back(layout);
    }

    VkDevice device = m_vulkandevice->logicalDevice;
    std::string vertShaderSpvFile = getAssetPath() + "shaders/glsl/hybridRayTracing/raytracing.vert.spv";
    Utils::ScopedShaderModule vertShaderModule(device,
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        vks::tools::loadShader(androidApp->activity->assetManager, vertShaderSpvFile.c_str(), device));
#else
        vks::tools::loadShader(vertShaderSpvFile.c_str(), device));
#endif

    RayShop::Vulkan::ShaderModule rtShaderModule;
    rtShaderModule.vkHandle = VK_NULL_HANDLE;

    RayShop::Vulkan::RayTracingShaderModuleCreateInfo rtShaderCreateInfo{};
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    std::string rtFragContent = Utils::GetFileContent(getAssetPath() + "shaders/glsl/hybridRayTracing/" + shaderName,
        androidApp->activity->assetManager);
#else
    std::string rtFragContent = Utils::GetFileContent(getAssetPath() + "shaders/glsl/hybridRayTracing/" + shaderName);
#endif

    std::vector<const char *> shaderIncludes;
    std::vector<const char *> shaderIncludeSources;
    shaderIncludes.push_back("common.glsl");
    shaderIncludes.push_back("raytracing.glsl");
    shaderIncludes.push_back("raytracing_color.frag");
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    std::string commonContent = Utils::GetFileContent(getAssetPath() + "shaders/glsl/hybridRayTracing/common.glsl",
        androidApp->activity->assetManager);
    std::string raytraceInclude = Utils::GetFileContent(
        getAssetPath() + "shaders/glsl/hybridRayTracing/raytracing.glsl", androidApp->activity->assetManager);
    std::string raytraceFragment = Utils::GetFileContent(
        getAssetPath() + "shaders/glsl/hybridRayTracing/raytracing_color.frag", androidApp->activity->assetManager);
#else
    std::string commonContent = Utils::GetFileContent(getAssetPath() + "shaders/glsl/hybridRayTracing/common.glsl");
    std::string raytraceInclude =
        Utils::GetFileContent(getAssetPath() + "shaders/glsl/hybridRayTracing/raytracing.glsl");
    std::string raytraceFragment =
        Utils::GetFileContent(getAssetPath() + "shaders/glsl/hybridRayTracing/raytracing_color.frag");
#endif
    shaderIncludeSources.push_back(commonContent.c_str());
    shaderIncludeSources.push_back(raytraceInclude.c_str());
    shaderIncludeSources.push_back(raytraceFragment.c_str());

    rtShaderCreateInfo.pName = "rtFragmentShader";
    rtShaderCreateInfo.pSource = rtFragContent.c_str();
    rtShaderCreateInfo.stage = RayShop::Vulkan::ShaderStage::SHADER_STAGE_FRAGMENT_BIT;
    rtShaderCreateInfo.hitFormat = RayShop::TraceRayHitFormat::T_PRIMID_INSTID_U_V;
    rtShaderCreateInfo.ppShaderIncludes = shaderIncludes.data();
    rtShaderCreateInfo.shaderIncludesCnt = static_cast<uint32_t>(shaderIncludes.size());
    rtShaderCreateInfo.ppShaderIncludeSources = shaderIncludeSources.data();
    RayShop::Result res = m_traversal->CreateRayTracingShaderModule(&rtShaderCreateInfo, &rtShaderModule);
    if (res != RayShop::Result::SUCCESS) {
        LOGE("%s: Failed to create raytracing shader module, err: %s.", __func__, m_traversal->GetErrorCodeString(res));
        return false;
    }

    Utils::ScopedShaderModule fragShaderModule(device, rtShaderModule.vkHandle);
    if (!vertShaderModule.Valid() || !fragShaderModule.Valid()) {
        LOGE("%s: Failed to create shader modules.", __func__);
        return false;
    }

    std::string entryPoint = "main";
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { Utils::PipelineShaderStageInfo(vertShaderModule.Get(),
        VK_SHADER_STAGE_VERTEX_BIT, entryPoint),
        Utils::PipelineShaderStageInfo(fragShaderModule.Get(), VK_SHADER_STAGE_FRAGMENT_BIT, entryPoint) };

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(1);
    colorBlendAttachments[0].blendEnable = VK_TRUE;
    colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachments[0].colorWriteMask = 0xf; // R|G|B|A
    m_rtPipeline->SetBlendAttachments(std::move(colorBlendAttachments));

    m_rtPipeline->SetDepthStencilState(
        detail::CreateDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL));

    std::vector<vkvert::VertexComponent> components = { vkvert::VertexComponent::Position,
        vkvert::VertexComponent::Normal, vkvert::VertexComponent::UV, vkvert::VertexComponent::Joint0,
        vkvert::VertexComponent::Weight0 };
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
            vkvert::Vertex::InputBindingDescription(0) };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes =
            vkvert::Vertex::InputAttributeDescriptions(0, components);
    m_rtPipeline->SetVertexInputBindings(vertexInputBindings);
    m_rtPipeline->SetVertexInputAttributes(vertexInputAttributes);
    if (!m_rtPipeline->Setup(rtSetLayouts, pushConstantRanges, m_framebuffer->renderPass, shaderStages,
        pipelineCache)) {
        return false;
    }

    VkDescriptorBufferInfo bvhTree {};
    VkDescriptorBufferInfo bvhTriangles {};
    VkDescriptorBufferInfo tlasInfo {};
    VkDescriptorBufferInfo rtCoreUniforms {};
    res = m_traversal->GetTraversalDescBufferInfos(&bvhTree, &bvhTriangles, &tlasInfo, &rtCoreUniforms);
    if (res != RayShop::Result::SUCCESS) {
        LOGE("%s: Failed to GetTraversalDescBufferInfos, err: %s.", __func__,
             RayShop::Vulkan::Traversal::GetErrorCodeString(res));
        return false;
    }

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = { Utils::WriteDescriptorSet(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, bvhTree),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, bvhTriangles),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, tlasInfo),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, rtCoreUniforms),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, m_uniformBuffers.matrices.descriptor),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, m_uniformBuffers.params.descriptor),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, m_bvhBuffers.vertex.descriptor),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7, m_bvhBuffers.index.descriptor),
        Utils::WriteDescriptorSet(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8, m_countBuffer.descriptor),
    };

    m_rtDescSet->Update(writeDescriptorSets);

    return true;
}

void RayTracingPass::SetupUniformBuffers()
{
    m_uniformBuffers.matrices.destroy();
    m_uniformBuffers.params.destroy();

    // pbr vertex shader uniform buffer
    VK_CHECK_RESULT(m_vulkandevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_uniformBuffers.matrices,
        sizeof(buf::UBOMatrices)));

    // Shared parameter uniform buffer
    VK_CHECK_RESULT(m_vulkandevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_uniformBuffers.params,
        sizeof(buf::UBOParams)));

    // Map persistent
    VK_CHECK_RESULT(m_uniformBuffers.params.map());
    VK_CHECK_RESULT(m_uniformBuffers.matrices.map());
}

void RayTracingPass::UpdateMatrices(const buf::UBOMatrices &uboMatrices)
{
    memcpy(m_uniformBuffers.matrices.mapped, &uboMatrices, sizeof(buf::UBOMatrices));
}

void RayTracingPass::UpdateParams(const buf::UBOParams &uboParams)
{
    memcpy(m_uniformBuffers.params.mapped, &uboParams, sizeof(buf::UBOParams));
}

VkRenderPassBeginInfo RayTracingPass::BuildRenderPassBeginInfo(const std::vector<VkClearValue> &clearValues)
{
    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = m_framebuffer->renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = m_width;
    renderPassBeginInfo.renderArea.extent.height = m_height;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();
    renderPassBeginInfo.framebuffer = m_framebuffer->framebuffer;
    return renderPassBeginInfo;
}

void RayTracingPass::SetViewport(VkCommandBuffer cmdBuffer)
{
    VkViewport viewport { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    VkRect2D scissor { { 0, 0 }, { m_width, m_height } };
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void RayTracingPass::RecordDepthOnlyCmd(VkCommandBuffer cmdBuffer, vkglTF::Model &scene)
{
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_depthOnlyPipeline->Handle());
    uint32_t bindSet = 0;
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_depthOnlyPipeline->GetPipelineLayout(),
        bindSet, 1, &m_depthOnlyDescSet->GetDescSet(), 0, nullptr);
    scene.draw(cmdBuffer, vkglTF::RenderFlags::BindUbo, m_depthOnlyPipeline->GetPipelineLayout(), bindSet + 2, 0);
}

void RayTracingPass::RecordRayTracingCmd(VkCommandBuffer cmdBuffer, vkibl::VulkanImageBasedLighting *ibl,
    vkglTF::Model &scene)
{
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_rtPipeline->Handle());
    uint32_t bindSet = 0;
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_rtPipeline->GetPipelineLayout(), bindSet, 1,
        &m_rtDescSet->GetDescSet(), 0, nullptr);
    ibl->BindDescriptorSet(cmdBuffer, m_rtPipeline->GetPipelineLayout(), bindSet + 1);
    scene.draw(cmdBuffer,
        vkglTF::RenderFlags::BindImages | vkglTF::RenderFlags::BindUbo | vkglTF::RenderFlags::PushMaterialConst,
        m_rtPipeline->GetPipelineLayout(), bindSet + 2);
}

void RayTracingPass::Draw(VkCommandBuffer cmdBuffer, vkibl::VulkanImageBasedLighting *ibl, vkglTF::Model &scene)
{
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;

    copyRegion.size = m_bvhStagingBuffers.vertex.size;
    vkCmdCopyBuffer(cmdBuffer, m_bvhStagingBuffers.vertex.buffer, m_bvhBuffers.vertex.buffer, 1, &copyRegion);
    copyRegion.size = m_bvhStagingBuffers.index.size;
    vkCmdCopyBuffer(cmdBuffer, m_bvhStagingBuffers.index.buffer, m_bvhBuffers.index.buffer, 1, &copyRegion);

    // reset count buffer to zero.
    if (m_showStat) {
        vkCmdFillBuffer(cmdBuffer, m_countBuffer.buffer, 0, m_countBuffer.size, 0);
    }

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassBeginInfo = BuildRenderPassBeginInfo(clearValues);

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    SetViewport(cmdBuffer);
    RecordDepthOnlyCmd(cmdBuffer, scene);
    RecordRayTracingCmd(cmdBuffer, ibl, scene);
    vkCmdEndRenderPass(cmdBuffer);

    Utils::BarrierInfo barrierInfo {};
    barrierInfo.srcMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    barrierInfo.dstMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    barrierInfo.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    barrierInfo.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    Utils::SetMemoryBarrier(cmdBuffer, barrierInfo);

    if (m_showStat) {
        copyRegion.size = m_countStagingBuffer.size;
        vkCmdCopyBuffer(cmdBuffer, m_countBuffer.buffer, m_countStagingBuffer.buffer, 1,
                        &copyRegion);
    }
}

float RayTracingPass::GetReflectArea() const
{
    uint32_t rtCount = *static_cast<uint32_t *>(m_countStagingBuffer.mapped);
    auto resolution = static_cast<float> (m_width * m_height);
    return static_cast<float> (rtCount) / resolution;
}
} // namespace rt
