/*
 * Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
 *
 * Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_set>

#include "vulkan/vulkan.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "BufferInfor.h"
#include "VulkanVertex.h"

#include <ktx.h>
#include <ktxvulkan.h>

#if defined(__ANDROID__)
#include "VulkanAndroid.h"
#include <android/asset_manager.h>
#endif

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u
#define PBR_WORKFLOW_METALLIC_ROUGHNESS 0.0f

namespace vkglTF {
enum DescriptorBindingFlags {
    ImageBaseColor = 0x00000001,
    ImageMetallicRoughness = 0x00000002,
    ImageNormalMap = 0x00000003,
    ImageOcclusion = 0x00000004,
    ImageEmissiveMap = 0x00000005
};

extern VkDescriptorSetLayout descriptorSetLayoutImage;
extern VkDescriptorSetLayout descriptorSetLayoutUbo;
extern VkMemoryPropertyFlags memoryPropertyFlags;
extern uint32_t descriptorBindingFlags;

struct Node;

/*
    glTF material class
*/
struct Material {
    vks::VulkanDevice *device;
    enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
    AlphaMode alphaMode = ALPHAMODE_OPAQUE;
    float alphaCutoff = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    glm::vec4 emissiveFactor = glm::vec4(1.0f);
    vks::Texture2D *baseColorTexture = nullptr;
    vks::Texture2D *metallicRoughnessTexture = nullptr;
    vks::Texture2D *normalTexture = nullptr;
    vks::Texture2D *occlusionTexture = nullptr;
    vks::Texture2D *emissiveTexture = nullptr;
    struct TexCoordSets {
        uint8_t baseColor = 0;
        uint8_t metallicRoughness = 0;
        uint8_t normal = 0;
        uint8_t occlusion = 0;
        uint8_t emissive = 0;
    } texCoordSets;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    Material(vks::VulkanDevice *device) : device(device){};
    void createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
                             uint32_t descriptorBindingFlags, vks::Texture2D &empty);
};

/*
    glTF primitive
*/
struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstVertex;
    uint32_t vertexCount;
    Material &material;

    struct Dimensions {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
        glm::vec3 size;
        glm::vec3 center;
        float radius;
    } dimensions;

    void setDimensions(glm::vec3 min, glm::vec3 max);
    Primitive(uint32_t firstIndex, uint32_t indexCount, Material &material)
        : firstIndex(firstIndex), indexCount(indexCount), material(material){};
};

/*
    glTF mesh
*/
struct Mesh {
    vks::VulkanDevice *device;

    std::vector<Primitive *> primitives;
    std::string name;
    uint32_t id;

    struct UniformBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDescriptorBufferInfo descriptor;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        void *mapped;
    } uniformBuffer;

    struct UniformBlock {
        glm::mat4 matrix;
        glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
        float jointcount{0};
    } uniformBlock;

    Mesh(vks::VulkanDevice *device, glm::mat4 matrix);
    ~Mesh();
};

/*
    glTF skin
*/
struct Skin {
    std::string name;
    Node *skeletonRoot = nullptr;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node *> joints;
};

/*
    glTF node
*/
struct Node {
    Node *parent;
    uint32_t index;
    std::vector<Node *> children;
    glm::mat4 matrix;
    std::string name;
    Mesh *mesh;
    Skin *skin;
    int32_t skinIndex = -1;
    glm::vec3 translation{};
    glm::vec3 scale{1.0f};
    glm::quat rotation{};
    glm::mat4 localMatrix();
    glm::mat4 getMatrix();
    void update();
    ~Node();
};

/*
    glTF animation channel
*/
struct AnimationChannel {
    enum PathType { TRANSLATION, ROTATION, SCALE };
    PathType path;
    Node *node;
    uint32_t samplerIndex;
};

/*
    glTF animation sampler
*/
struct AnimationSampler {
    enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
    InterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputsVec4;
};

/*
    glTF animation
*/
struct Animation {
    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
};

enum FileLoadingFlags { None = 0x00000000,
                        PreMultiplyVertexColors = 0x00000002,
                        DontLoadImages = 0x00000004,
                        PreConvert = 0x00000008};

enum RenderFlags {
    BindImages = 0x00000001,
    RenderOpaqueNodes = 0x00000002,
    RenderAlphaMaskedNodes = 0x00000004,
    RenderAlphaBlendedNodes = 0x00000008,
    BindUbo = 0x00000010,
    PushMaterialConst = 0x00000020,
    RenderReflection = 0x00000040
};

/*
    glTF model loading and rendering class
*/
class Model {
public:
    // triangle vertices and indices for build bvh
    std::vector<uint32_t> indexBuffer;
    std::vector<vkvert::Vertex> vertexBuffer;

private:
    vks::Texture2D *getTexture(uint32_t index);
    vks::Texture2D emptyTexture;
    void collectRelectionInformations(Node *node);

public:
    vks::VulkanDevice *device;
    VkDescriptorPool descriptorPool;

    struct {
        vks::Buffer vertices;
        vks::Buffer indices;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    } vertexIndexBufers;

    std::vector<Node *> nodes;
    // which node will show
    std::unordered_set<std::string> drawMeshNames{};
    std::unordered_set<std::string> allMeshNames;
    uint32_t globMeshId = 0;

    // Store all the primitive informations, uniform buffer and node id of relection nodes
    // Primitive contains vertex information for relection node;
    // Mesh::UniformBuffer contains the model and skinned matrix;
    std::vector<std::tuple<Primitive *, Mesh::UniformBuffer *, uint32_t>> reflectionPrimitiveIds{};
    // Store all the materials of scene and node id for calculating the reflection color
    // node id is used to filter the hit informations which are different with the binding material
    std::vector<std::pair<vkglTF::Material *, uint32_t>> sceneMaterialIds{};
    std::vector<Node *> linearNodes;

    std::vector<Skin *> skins;

    std::vector<vks::Texture2D> textures;
    std::vector<vks::TextureSampler> textureSamplers;
    std::vector<Material> materials;
    std::vector<Animation> animations;

    struct Dimensions {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
        glm::vec3 size;
        glm::vec3 center;
        float radius;
    } dimensions;

    bool metallicRoughnessWorkflow = true;
    bool buffersBound = false;
    std::string path;

    Model(){};
    ~Model();
    void setDrawMeshNames(std::unordered_set<std::string> meshNames);
    void loadNode(vkglTF::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model,
                  std::vector<uint32_t> &indexBuffer, std::vector<vkvert::Vertex> &vertexBuffer, float globalscale);
    void loadSkins(tinygltf::Model &gltfModel);
    void loadTextures(tinygltf::Model &gltfModel, vks::VulkanDevice *device, VkQueue transferQueue);
    VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
    VkFilter getVkFilterMode(int32_t filterMode);
    void loadTextureSamplers(tinygltf::Model &gltfModel);

    void loadMaterials(tinygltf::Model &gltfModel);
    void loadAnimations(tinygltf::Model &gltfModel);
    void loadFromFile(std::string filename, vks::VulkanDevice *device, VkQueue transferQueue,
                      uint32_t fileLoadingFlags = vkglTF::FileLoadingFlags::None, float scale = 1.0f);
    void prepareVulkanModel(vks::VulkanDevice *device, VkQueue transferQueue);
    void convertLocalVertexToWorld(const glm::mat4 modelMatrix, std::vector<vkvert::Vertex> &outWorldVertices);
    void bindBuffers(VkCommandBuffer commandBuffer);
    void drawNode(Node *node, VkCommandBuffer commandBuffer, uint32_t renderFlags = 0,
                  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindSet = 1);
    void drawReflection(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0,
                        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindSet = 1);
    void drawPrimitive(Primitive *primitive, Mesh::UniformBuffer *uniformBuffer, Material *material,
                       VkCommandBuffer commandBuffer, uint32_t renderFlags = 0,
                       VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindSet = 1, uint32_t meshId = 0);
    void draw(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE,
              uint32_t bindSet = 1, uint32_t nodeId = 0);
    void getNodeDimensions(Node *node, glm::vec3 &min, glm::vec3 &max);
    void getSceneDimensions();
    void updateAnimation(uint32_t index, float time);
    Node *findNode(Node *parent, uint32_t index);
    Node *nodeFromIndex(uint32_t index);
    void prepareNodeDescriptor(vkglTF::Node *node, VkDescriptorSetLayout descriptorSetLayout);
};
} // namespace vkglTF