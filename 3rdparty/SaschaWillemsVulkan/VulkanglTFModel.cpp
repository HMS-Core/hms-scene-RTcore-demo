
/*
 * Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
 *
 * Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif

#include "VulkanglTFModel.h"

VkDescriptorSetLayout vkglTF::descriptorSetLayoutImage = VK_NULL_HANDLE;
VkDescriptorSetLayout vkglTF::descriptorSetLayoutUbo = VK_NULL_HANDLE;
VkMemoryPropertyFlags vkglTF::memoryPropertyFlags = 0;
uint32_t vkglTF::descriptorBindingFlags = vkglTF::DescriptorBindingFlags::ImageBaseColor;

/*
    glTF material
*/
void vkglTF::Material::createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
                                           uint32_t descriptorBindingFlags, vks::Texture2D &empty)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &descriptorSet));
    // We just support metallicRoughness pbrWorkflow
    std::vector<VkDescriptorImageInfo> imageDescriptors = {
        baseColorTexture != nullptr ? baseColorTexture->descriptor : empty.descriptor,
        metallicRoughnessTexture != nullptr ? metallicRoughnessTexture->descriptor : empty.descriptor,
        normalTexture != nullptr ? normalTexture->descriptor : empty.descriptor,
        occlusionTexture != nullptr ? occlusionTexture->descriptor : empty.descriptor,
        emissiveTexture != nullptr ? emissiveTexture->descriptor : empty.descriptor};

    std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
    for (size_t i = 0; i < imageDescriptors.size(); i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].descriptorCount = 1;
        writeDescriptorSets[i].dstSet = descriptorSet;
        writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
        writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
    }

    vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);
}

/*
    glTF primitive
*/
void vkglTF::Primitive::setDimensions(glm::vec3 min, glm::vec3 max)
{
    dimensions.min = min;
    dimensions.max = max;
    dimensions.size = max - min;
    dimensions.center = (min + max) / 2.0f;
    dimensions.radius = glm::distance(min, max) / 2.0f;
}

/*
    glTF mesh
*/
vkglTF::Mesh::Mesh(vks::VulkanDevice *device, glm::mat4 matrix)
{
    this->device = device;
    this->uniformBlock.matrix = matrix;
    VK_CHECK_RESULT(device->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sizeof(uniformBlock), &uniformBuffer.buffer, &uniformBuffer.memory, &uniformBlock));
    VK_CHECK_RESULT(
        vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
    uniformBuffer.descriptor = {uniformBuffer.buffer, 0, sizeof(uniformBlock)};
};

vkglTF::Mesh::~Mesh()
{
    vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
    vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
}

/*
    glTF node
*/
glm::mat4 vkglTF::Node::localMatrix()
{
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) *
           matrix;
}

glm::mat4 vkglTF::Node::getMatrix()
{
    glm::mat4 m = localMatrix();
    vkglTF::Node *p = parent;
    while (p) {
        m = p->localMatrix() * m;
        p = p->parent;
    }
    return m;
}

void vkglTF::Node::update()
{
    if (mesh) {
        glm::mat4 m = getMatrix();
        mesh->uniformBlock.matrix = m;
        if (skin) {
            // Update join matrices
            glm::mat4 inverseTransform = glm::inverse(m);
            for (size_t i = 0; i < skin->joints.size(); i++) {
                vkglTF::Node *jointNode = skin->joints[i];
                glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
                jointMat = inverseTransform * jointMat;
                mesh->uniformBlock.jointMatrix[i] = jointMat;
            }
            mesh->uniformBlock.jointcount = (float)skin->joints.size();
            memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
        } else {
            memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
        }
    }

    for (auto &child : children) {
        child->update();
    }
}

vkglTF::Node::~Node()
{
    if (mesh) {
        delete mesh;
    }
    for (auto &child : children) {
        delete child;
    }
}

vks::Texture2D *vkglTF::Model::getTexture(uint32_t index)
{

    if (index < textures.size()) {
        return &textures[index];
    }
    return nullptr;
}

/*
    glTF model loading and rendering class
*/
vkglTF::Model::~Model()
{
    vertexIndexBufers.vertices.destroy();
    vertexIndexBufers.indices.destroy();
    for (auto texture : textures) {
        texture.destroy();
    }
    for (auto node : nodes) {
        delete node;
    }
    if (descriptorSetLayoutUbo != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayoutUbo, nullptr);
        descriptorSetLayoutUbo = VK_NULL_HANDLE;
    }
    if (descriptorSetLayoutImage != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayoutImage, nullptr);
        descriptorSetLayoutImage = VK_NULL_HANDLE;
    }

    vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
    emptyTexture.destroy();
}

void vkglTF::Model::loadNode(vkglTF::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex,
                             const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer,
                             std::vector<vkvert::Vertex> &vertexBuffer, float globalscale)
{
    vkglTF::Node *newNode = new Node{};
    newNode->index = nodeIndex;
    newNode->parent = parent;
    newNode->name = node.name;
    newNode->skinIndex = node.skin;
    newNode->matrix = glm::mat4(1.0f);

    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
    if (node.translation.size() == 3) {
        translation = glm::make_vec3(node.translation.data());
        newNode->translation = translation;
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4) {
        glm::quat q = glm::make_quat(node.rotation.data());
        newNode->rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3) {
        scale = glm::make_vec3(node.scale.data());
        newNode->scale = scale;
    }
    if (node.matrix.size() == 16) {
        newNode->matrix = glm::make_mat4x4(node.matrix.data());
        if (globalscale != 1.0f) {
            // newNode->matrix = glm::scale(newNode->matrix, glm::vec3(globalscale));
        }
    };

    // Node with children
    if (node.children.size() > 0) {
        for (auto i = 0; i < node.children.size(); i++) {
            loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer,
                     globalscale);
        }
    }

    // Node contains mesh data
    if (node.mesh > -1) {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        Mesh *newMesh = new Mesh(device, newNode->matrix);
        newMesh->name = mesh.name;
        newMesh->id = globMeshId++;
        allMeshNames.insert(mesh.name);
        drawMeshNames.insert(mesh.name);
        for (size_t j = 0; j < mesh.primitives.size(); j++) {
            const tinygltf::Primitive &primitive = mesh.primitives[j];
            if (primitive.indices < 0) {
                continue;
            }
            uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
            glm::vec3 posMin{};
            glm::vec3 posMax{};
            bool hasSkin = false;
            // Vertices
            {
                const float *bufferPos = nullptr;
                const float *bufferNormals = nullptr;
                const float *bufferTexCoords = nullptr;
                const float *bufferColors = nullptr;
                const float *bufferTangents = nullptr;
                uint32_t numColorComponents;
                const uint16_t *bufferJoints = nullptr;
                const float *bufferWeights = nullptr;

                // Position attribute is required
                assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
                bufferPos = reinterpret_cast<const float *>(
                    &(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

                if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                    const tinygltf::Accessor &normAccessor =
                        model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
                    bufferNormals = reinterpret_cast<const float *>(
                        &(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                }

                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &uvAccessor =
                        model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoords = reinterpret_cast<const float *>(
                        &(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                }

                if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &colorAccessor =
                        model.accessors[primitive.attributes.find("COLOR_0")->second];
                    const tinygltf::BufferView &colorView = model.bufferViews[colorAccessor.bufferView];
                    // Color buffer are either of type vec3 or vec4
                    numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                    bufferColors = reinterpret_cast<const float *>(
                        &(model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
                }

                if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                    const tinygltf::Accessor &tangentAccessor =
                        model.accessors[primitive.attributes.find("TANGENT")->second];
                    const tinygltf::BufferView &tangentView = model.bufferViews[tangentAccessor.bufferView];
                    bufferTangents = reinterpret_cast<const float *>(
                        &(model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
                }

                // Skinning
                // Joints
                if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &jointAccessor =
                        model.accessors[primitive.attributes.find("JOINTS_0")->second];
                    const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
                    bufferJoints = reinterpret_cast<const uint16_t *>(
                        &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
                }

                if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &uvAccessor =
                        model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferWeights = reinterpret_cast<const float *>(
                        &(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                }

                hasSkin = (bufferJoints && bufferWeights);

                vertexCount = static_cast<uint32_t>(posAccessor.count);

                for (size_t v = 0; v < posAccessor.count; v++) {
                    vkvert::Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), float(newMesh->id));
                    vert.normal =
                        glm::vec4(glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3])
                                                                         : glm::vec3(0.0f))),
                                  0.0);
                    vert.uv = glm::vec4(bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f),
                                        0.f, 0.f);
                    if (bufferColors) {
                        switch (numColorComponents) {
                            case 3:
                                vert.color = glm::vec4(glm::make_vec3(&bufferColors[v * 3]), 1.0f);
                            case 4:
                                vert.color = glm::make_vec4(&bufferColors[v * 4]);
                        }
                    } else {
                        vert.color = glm::vec4(1.0f);
                    }
                    vert.tangent = bufferTangents ? glm::vec4(glm::make_vec4(&bufferTangents[v * 4])) : glm::vec4(0.0f);
                    vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * 4])) : glm::vec4(0.0f);
                    vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * 4]) : glm::vec4(0.0f);
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            {
                const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);

                switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        uint32_t *buf = new uint32_t[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                               accessor.count * sizeof(uint32_t));
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        uint16_t *buf = new uint16_t[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                               accessor.count * sizeof(uint16_t));
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        uint8_t *buf = new uint8_t[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                               accessor.count * sizeof(uint8_t));
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!"
                                  << std::endl;
                        return;
                }
            }
            Primitive *newPrimitive = new Primitive(
                indexStart, indexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
            newPrimitive->firstVertex = vertexStart;
            newPrimitive->vertexCount = vertexCount;
            newPrimitive->setDimensions(posMin, posMax);
            newMesh->primitives.push_back(newPrimitive);
        }
        newNode->mesh = newMesh;
    }
    if (parent) {
        parent->children.push_back(newNode);
    } else {
        nodes.push_back(newNode);
    }
    linearNodes.push_back(newNode);
}

void vkglTF::Model::loadSkins(tinygltf::Model &gltfModel)
{
    for (tinygltf::Skin &source : gltfModel.skins) {
        Skin *newSkin = new Skin{};
        newSkin->name = source.name;

        // Find skeleton root node
        if (source.skeleton > -1) {
            newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
        }

        // Find joint nodes
        for (int jointIndex : source.joints) {
            Node *node = nodeFromIndex(jointIndex);
            if (node) {
                newSkin->joints.push_back(nodeFromIndex(jointIndex));
            }
        }

        // Get inverse bind matrices from buffer
        if (source.inverseBindMatrices > -1) {
            const tinygltf::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
            const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
            newSkin->inverseBindMatrices.resize(accessor.count);
            memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                   accessor.count * sizeof(glm::mat4));
        }

        skins.push_back(newSkin);
    }
}

void vkglTF::Model::loadTextures(tinygltf::Model &gltfModel, vks::VulkanDevice *device, VkQueue transferQueue)
{
    for (tinygltf::Texture &tex : gltfModel.textures) {
        tinygltf::Image image = gltfModel.images[tex.source];
        vks::TextureSampler textureSampler;
        if (tex.sampler == -1) {
            // No sampler specified, use a default one
            textureSampler.magFilter = VK_FILTER_LINEAR;
            textureSampler.minFilter = VK_FILTER_LINEAR;
            textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else {
            textureSampler = textureSamplers[tex.sampler];
        }
        vks::Texture2D texture;
        texture.fromglTfImage(image, textureSampler, device, transferQueue);
        textures.push_back(texture);
    }

    // Create an empty texture to be used for empty material images
    emptyTexture.createEmptyTexture(VK_FORMAT_R8G8B8A8_UNORM, device, transferQueue, VK_FILTER_LINEAR,
                                    VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

VkSamplerAddressMode vkglTF::Model::getVkWrapMode(int32_t wrapMode)
{
    switch (wrapMode) {
        case 10497:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case 33071:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 33648:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkFilter vkglTF::Model::getVkFilterMode(int32_t filterMode)
{
    switch (filterMode) {
        case 9728:
            return VK_FILTER_NEAREST;
        case 9729:
            return VK_FILTER_LINEAR;
        case 9984:
            return VK_FILTER_NEAREST;
        case 9985:
            return VK_FILTER_NEAREST;
        case 9986:
            return VK_FILTER_LINEAR;
        case 9987:
            return VK_FILTER_LINEAR;
        default:
            return VK_FILTER_LINEAR;
    }
}

void vkglTF::Model::loadTextureSamplers(tinygltf::Model &gltfModel)
{
    for (tinygltf::Sampler smpl : gltfModel.samplers) {
        vks::TextureSampler sampler{};
        sampler.minFilter = getVkFilterMode(smpl.minFilter);
        sampler.magFilter = getVkFilterMode(smpl.magFilter);
        sampler.addressModeU = getVkWrapMode(smpl.wrapS);
        sampler.addressModeV = getVkWrapMode(smpl.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        textureSamplers.push_back(sampler);
    }
}

void vkglTF::Model::loadMaterials(tinygltf::Model &gltfModel)
{
    for (tinygltf::Material &mat : gltfModel.materials) {
        vkglTF::Material material(device);
        if (mat.values.find("baseColorTexture") != mat.values.end()) {
            material.baseColorTexture =
                getTexture(gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source);
            material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
        }
        // Metallic roughness workflow
        if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
            material.metallicRoughnessTexture =
                getTexture(gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source);
            material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
        }
        if (mat.values.find("roughnessFactor") != mat.values.end()) {
            material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
        }
        if (mat.values.find("metallicFactor") != mat.values.end()) {
            material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
        }
        if (mat.values.find("baseColorFactor") != mat.values.end()) {
            material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
        }
        if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
            material.normalTexture =
                getTexture(gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source);
            material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
            material.emissiveTexture =
                getTexture(gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source);
            material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
            material.occlusionTexture =
                getTexture(gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source);
            material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
            tinygltf::Parameter param = mat.additionalValues["alphaMode"];
            if (param.string_value == "BLEND") {
                material.alphaMode = Material::ALPHAMODE_BLEND;
            }
            if (param.string_value == "MASK") {
                material.alphaMode = Material::ALPHAMODE_MASK;
            }
        }
        if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
            material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
        }
        if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
            material.emissiveFactor =
                glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
            material.emissiveFactor = glm::vec4(0.0f);
        }

        materials.push_back(material);
    }
    // Push a default material at the end of the list for meshes with no material assigned
    materials.push_back(Material(device));
}

void vkglTF::Model::loadAnimations(tinygltf::Model &gltfModel)
{
    for (tinygltf::Animation &anim : gltfModel.animations) {
        vkglTF::Animation animation{};
        animation.name = anim.name;
        if (anim.name.empty()) {
            animation.name = std::to_string(animations.size());
        }

        // Samplers
        for (auto &samp : anim.samplers) {
            vkglTF::AnimationSampler sampler{};

            if (samp.interpolation == "LINEAR") {
                sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
            }
            if (samp.interpolation == "STEP") {
                sampler.interpolation = AnimationSampler::InterpolationType::STEP;
            }
            if (samp.interpolation == "CUBICSPLINE") {
                sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
            }

            // Read sampler input time values
            {
                const tinygltf::Accessor &accessor = gltfModel.accessors[samp.input];
                const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                float *buf = new float[accessor.count];
                memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(float));
                for (size_t index = 0; index < accessor.count; index++) {
                    sampler.inputs.push_back(buf[index]);
                }

                for (auto input : sampler.inputs) {
                    if (input < animation.start) {
                        animation.start = input;
                    };
                    if (input > animation.end) {
                        animation.end = input;
                    }
                }
            }

            // Read sampler output T/R/S values
            {
                const tinygltf::Accessor &accessor = gltfModel.accessors[samp.output];
                const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                switch (accessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                        glm::vec3 *buf = new glm::vec3[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                               accessor.count * sizeof(glm::vec3));
                        for (size_t index = 0; index < accessor.count; index++) {
                            sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        glm::vec4 *buf = new glm::vec4[accessor.count];
                        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                               accessor.count * sizeof(glm::vec4));
                        for (size_t index = 0; index < accessor.count; index++) {
                            sampler.outputsVec4.push_back(buf[index]);
                        }
                        break;
                    }
                    default: {
                        std::cout << "unknown type" << std::endl;
                        break;
                    }
                }
            }

            animation.samplers.push_back(sampler);
        }

        // Channels
        for (auto &source : anim.channels) {
            vkglTF::AnimationChannel channel{};

            if (source.target_path == "rotation") {
                channel.path = AnimationChannel::PathType::ROTATION;
            }
            if (source.target_path == "translation") {
                channel.path = AnimationChannel::PathType::TRANSLATION;
            }
            if (source.target_path == "scale") {
                channel.path = AnimationChannel::PathType::SCALE;
            }
            if (source.target_path == "weights") {
                std::cout << "weights not yet supported, skipping channel" << std::endl;
                continue;
            }
            channel.samplerIndex = source.sampler;
            channel.node = nodeFromIndex(source.target_node);
            if (!channel.node) {
                continue;
            }

            animation.channels.push_back(channel);
        }

        animations.push_back(animation);
    }
}

void vkglTF::Model::convertLocalVertexToWorld(const glm::mat4 modelMatrix,
                                              std::vector<vkvert::Vertex> &outWorldVertices)
{
    if (outWorldVertices.size() != vertexBuffer.size()) {
        outWorldVertices.clear();
        outWorldVertices.resize(vertexBuffer.size());
    }
    for (Node *node : linearNodes) {
        if (node->mesh) {
            const glm::mat4 localMatrix = node->getMatrix();
            for (Primitive *primitive : node->mesh->primitives) {
                for (uint32_t i = 0; i < primitive->vertexCount; i++) {
                    vkvert::Vertex vertex = vertexBuffer[primitive->firstVertex + i];
                    // Pre-transform vertex positions by node-hierarchy
                    // TODO: Skin need to be implemented;
                    vertex.pos = modelMatrix * localMatrix * glm::vec4(glm::vec3(vertex.pos), 1.0f);
                    // Flip Y-Axis of vertex positions
                    vertex.pos.y *= -1.0f;
                    vertex.pos = glm::vec4(vertex.pos.x / vertex.pos.w, vertex.pos.y / vertex.pos.w,
                                           vertex.pos.z / vertex.pos.w, 1.f);
                    vertex.normal =
                        glm::vec4(glm::normalize(glm::transpose(glm::inverse(glm::mat3(modelMatrix * localMatrix))) *
                                                 glm::normalize(glm::vec3(vertex.normal))),
                                  0.0);

                    outWorldVertices[primitive->firstVertex + i] = vertex;
                }
            }
        }
    }
}

void vkglTF::Model::prepareVulkanModel(vks::VulkanDevice *device, VkQueue transferQueue)
{
    size_t vertexBufferSize = vertexBuffer.size() * sizeof(vkvert::Vertex);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

    assert((vertexBufferSize > 0) && (indexBufferSize > 0));

    device->createBufferWithStagigingBuffer(
        transferQueue, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexIndexBufers.vertices, vertexBufferSize,
        (void *)vertexBuffer.data());

    device->createBufferWithStagigingBuffer(
        transferQueue, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexIndexBufers.indices, indexBufferSize, (void *)indexBuffer.data());

    // Setup descriptors
    uint32_t meshCount{0};
    uint32_t imageCount{0};
    uint32_t materialCount{0};
    for (auto node : linearNodes) {
        if (node->mesh) {
            meshCount++;
        }
    }
    for (auto material : materials) {
        // every material contains 5 texture for pbr rendering
        imageCount += 5;
        materialCount++;
    }
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, meshCount},
                                                   {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount}};

    VkDescriptorPoolCreateInfo descriptorPoolCI{};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCI.pPoolSizes = poolSizes.data();
    // 1: indices and vertices descriptorset
    descriptorPoolCI.maxSets = meshCount + materialCount + 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool));

    // Descriptors for per-node uniform buffers
    {
        // Layout is global, so only create if it hasn't already been created before
        if (descriptorSetLayoutUbo == VK_NULL_HANDLE) {
            std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                vks::initializers::descriptorSetLayoutBinding(
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            };
            VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
            descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
            descriptorLayoutCI.pBindings = setLayoutBindings.data();
            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr,
                                                        &descriptorSetLayoutUbo));
        }
        for (auto node : nodes) {
            prepareNodeDescriptor(node, descriptorSetLayoutUbo);
        }
    }

    // Descriptors for per-material images
    {
        // Layout is global, so only create if it hasn't already been created before
        if (descriptorSetLayoutImage == VK_NULL_HANDLE) {
            std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            };
            VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
            descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
            descriptorLayoutCI.pBindings = setLayoutBindings.data();
            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr,
                                                        &descriptorSetLayoutImage));
        }
        for (auto &material : materials) {
            material.createDescriptorSet(descriptorPool, vkglTF::descriptorSetLayoutImage, descriptorBindingFlags,
                                         emptyTexture);
        }
    }
}

void vkglTF::Model::loadFromFile(std::string filename, vks::VulkanDevice *device, VkQueue transferQueue,
                                 uint32_t fileLoadingFlags, float scale)
{
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

#if defined(__ANDROID__)
    // On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset
    // manager We let tinygltf handle this, by passing the asset manager of our app
    tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
    size_t pos = filename.find_last_of('/');
    path = filename.substr(0, pos);

    this->device = device;
    bool binary = false;
    size_t extpos = filename.rfind('.', filename.length());
    if (extpos != std::string::npos) {
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
    }

    bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str())
                             : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

    indexBuffer.clear();
    vertexBuffer.clear();

    if (fileLoaded) {
        if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages)) {
            loadTextureSamplers(gltfModel);
            loadTextures(gltfModel, device, transferQueue);
        }
        loadMaterials(gltfModel);
        const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
        // Reset Global Mesh id
        globMeshId = 0;
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
            loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
        }
        if (gltfModel.animations.size() > 0) {
            loadAnimations(gltfModel);
        }
        loadSkins(gltfModel);

        for (auto node : linearNodes) {
            // Assign skins
            if (node->skinIndex > -1) {
                node->skin = skins[node->skinIndex];
            }
            // Initial pose
            if (node->mesh) {
                node->update();
            }
        }
    } else {
        // TODO: throw
        vks::tools::exitFatal("Could not load glTF file \"" + filename + "\": " + error, -1);
        return;
    }

    // Pre-Calculations for requested features
    if (fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors) {
        const bool preMultiplyColor = fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors;
        const bool preConvert = fileLoadingFlags & FileLoadingFlags::PreConvert;
        for (Node *node : linearNodes) {
            if (node->mesh) {
                const glm::mat4 localMatrix = node->getMatrix();
                for (Primitive *primitive : node->mesh->primitives) {
                    for (uint32_t i = 0; i < primitive->vertexCount; i++) {
                        vkvert::Vertex &vertex = vertexBuffer[primitive->firstVertex + i];
                        // Pre-Multiply vertex colors with material base color
                        if (preMultiplyColor) {
                            vertex.color = primitive->material.baseColorFactor * vertex.color;
                        }
                    }
                }
            }
        }
    }

    for (auto extension : gltfModel.extensionsUsed) {
        if (extension == "KHR_materials_pbrSpecularGlossiness") {
            std::cout << "Required extension: " << extension;
            metallicRoughnessWorkflow = false;
        }
    }
    getSceneDimensions();
    prepareVulkanModel(device, transferQueue);
}

void vkglTF::Model::bindBuffers(VkCommandBuffer commandBuffer)
{
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexIndexBufers.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vertexIndexBufers.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    buffersBound = true;
}

void vkglTF::Model::collectRelectionInformations(Node *node)
{
    if (node->mesh) {
        for (Primitive *primitive : node->mesh->primitives) {
            // We just support the  reflection of Opaque objects
            uint32_t nodeId = node->mesh->id;
            if (primitive->material.alphaMode == Material::ALPHAMODE_OPAQUE) {
                sceneMaterialIds.push_back(std::make_pair(&primitive->material, nodeId));
            }

            if (drawMeshNames.find(node->name) != drawMeshNames.end()) {
                reflectionPrimitiveIds.push_back(std::make_tuple(primitive, &node->mesh->uniformBuffer, nodeId));
            }
        }
    }
    for (auto &child : node->children) {
        collectRelectionInformations(child);
    }
}

void vkglTF::Model::drawPrimitive(Primitive *primitive, Mesh::UniformBuffer *uniformBuffer, Material *material,
                                  VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                                  uint32_t bindSet, uint32_t meshId)
{

    if (renderFlags & RenderFlags::BindImages) {
        assert(material != nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindSet, 1,
                                &material->descriptorSet, 0, nullptr);
    }
    if (renderFlags & RenderFlags::BindUbo) {
        assert(uniformBuffer != nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindSet + 1, 1,
                                &uniformBuffer->descriptorSet, 0, nullptr);
    }

    if (renderFlags & RenderFlags::PushMaterialConst) {
        assert(material != nullptr);
        // Pass material parameters as push constants
        buf::PushConstBlockMaterial pushConstBlockMaterial{};
        // Pass material parameters as push constants
        pushConstBlockMaterial.emissiveFactor = material->emissiveFactor;
        // To save push constant space, availabilty and texture coordiante set are combined
        // -1 = texture not used for this material, >= 0 texture used and index of texture coordinate set
        pushConstBlockMaterial.colorTextureSet =
            material->baseColorTexture != nullptr ? material->texCoordSets.baseColor : -1;
        pushConstBlockMaterial.normalTextureSet =
            material->normalTexture != nullptr ? material->texCoordSets.normal : -1;
        pushConstBlockMaterial.occlusionTextureSet =
            material->occlusionTexture != nullptr ? material->texCoordSets.occlusion : -1;
        pushConstBlockMaterial.emissiveTextureSet =
            material->emissiveTexture != nullptr ? material->texCoordSets.emissive : -1;
        pushConstBlockMaterial.alphaMask = static_cast<float>(material->alphaMode == vkglTF::Material::ALPHAMODE_MASK);
        pushConstBlockMaterial.alphaMaskCutoff = material->alphaCutoff;

        // Metallic roughness workflow
        pushConstBlockMaterial.workflow = static_cast<float>(PBR_WORKFLOW_METALLIC_ROUGHNESS);
        pushConstBlockMaterial.baseColorFactor = material->baseColorFactor;
        pushConstBlockMaterial.metallicFactor = material->metallicFactor;
        pushConstBlockMaterial.roughnessFactor = material->roughnessFactor;
        pushConstBlockMaterial.PhysicalDescriptorTextureSet =
            material->metallicRoughnessTexture != nullptr ? material->texCoordSets.metallicRoughness : -1;
        pushConstBlockMaterial.colorTextureSet =
            material->baseColorTexture != nullptr ? material->texCoordSets.baseColor : -1;
        // node id starts from 0
        pushConstBlockMaterial.meshId = meshId;
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(buf::PushConstBlockMaterial), &pushConstBlockMaterial);
    }

    vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
}

void vkglTF::Model::drawReflection(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                                   uint32_t bindSet)
{
    if (reflectionPrimitiveIds.empty()) {
        return;
    }
    for (auto &reflectionPrimitiveId : reflectionPrimitiveIds) {
        Primitive *primitive = nullptr;
        Mesh::UniformBuffer *uniformBuffer = nullptr;
        uint32_t currentNodeId = 0;
        std::tie(primitive, uniformBuffer, currentNodeId) = reflectionPrimitiveId;
        if ((renderFlags & RenderFlags::BindImages) | (renderFlags & RenderFlags::PushMaterialConst)) {
            for (auto &sceneMaterialId : sceneMaterialIds) {
                Material *hitMaterial = sceneMaterialId.first;
                uint32_t hitNodeId = sceneMaterialId.second;
                drawPrimitive(primitive, uniformBuffer, hitMaterial, commandBuffer, renderFlags, pipelineLayout,
                              bindSet, hitNodeId);
            }
        } else {
            drawPrimitive(primitive, uniformBuffer, nullptr, commandBuffer, renderFlags, pipelineLayout, bindSet,
                          currentNodeId);
        }
    }
}

void vkglTF::Model::drawNode(Node *node, VkCommandBuffer commandBuffer, uint32_t renderFlags,
                             VkPipelineLayout pipelineLayout, uint32_t bindSet)
{
    if (node->mesh) {
        uint32_t meshId = node->mesh->id;
        for (Primitive *primitive : node->mesh->primitives) {
            bool skip = false;
            const vkglTF::Material &material = primitive->material;
            if (renderFlags & RenderFlags::RenderOpaqueNodes) {
                skip = (material.alphaMode != Material::ALPHAMODE_OPAQUE);
            }
            if (renderFlags & RenderFlags::RenderAlphaMaskedNodes) {
                skip = (material.alphaMode != Material::ALPHAMODE_MASK);
            }
            if (renderFlags & RenderFlags::RenderAlphaBlendedNodes) {
                skip = (material.alphaMode != Material::ALPHAMODE_BLEND);
            }
            if (drawMeshNames.find(node->mesh->name) == drawMeshNames.end()) {
                skip = true;
            }

            if (!skip) {
                drawPrimitive(primitive, &node->mesh->uniformBuffer, &primitive->material, commandBuffer, renderFlags,
                              pipelineLayout, bindSet, meshId);
            }
        }
    }
    for (auto &child : node->children) {
        drawNode(child, commandBuffer, renderFlags, pipelineLayout, bindSet);
    }
}

void vkglTF::Model::draw(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout,
                         uint32_t bindSet, uint32_t nodeId)
{
    if (!buffersBound) {
        const VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexIndexBufers.vertices.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, vertexIndexBufers.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    if (renderFlags & RenderFlags::RenderReflection) {
        reflectionPrimitiveIds.clear();
        sceneMaterialIds.clear();
        for (auto &node : nodes) {
            collectRelectionInformations(node);
        }
        // Draw reflection
        drawReflection(commandBuffer, renderFlags, pipelineLayout, bindSet);
    } else {
        // Regular rendering
        for (auto &node : nodes) {
            drawNode(node, commandBuffer, renderFlags, pipelineLayout, bindSet);
        }
    }
}

void vkglTF::Model::getNodeDimensions(Node *node, glm::vec3 &min, glm::vec3 &max)
{
    if (node->mesh) {
        for (Primitive *primitive : node->mesh->primitives) {
            glm::vec4 locMin = glm::vec4(primitive->dimensions.min, 1.0f) * node->getMatrix();
            glm::vec4 locMax = glm::vec4(primitive->dimensions.max, 1.0f) * node->getMatrix();
            if (locMin.x < min.x) {
                min.x = locMin.x;
            }
            if (locMin.y < min.y) {
                min.y = locMin.y;
            }
            if (locMin.z < min.z) {
                min.z = locMin.z;
            }
            if (locMax.x > max.x) {
                max.x = locMax.x;
            }
            if (locMax.y > max.y) {
                max.y = locMax.y;
            }
            if (locMax.z > max.z) {
                max.z = locMax.z;
            }
        }
    }
    for (auto child : node->children) {
        getNodeDimensions(child, min, max);
    }
}

void vkglTF::Model::getSceneDimensions()
{
    dimensions.min = glm::vec3(FLT_MAX);
    dimensions.max = glm::vec3(-FLT_MAX);
    for (auto node : nodes) {
        getNodeDimensions(node, dimensions.min, dimensions.max);
    }
    dimensions.size = dimensions.max - dimensions.min;
    dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
    dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}

void vkglTF::Model::updateAnimation(uint32_t index, float time)
{
    if (index > static_cast<uint32_t>(animations.size()) - 1) {
        std::cout << "No animation with index " << index << std::endl;
        return;
    }
    Animation &animation = animations[index];

    bool updated = false;
    for (auto &channel : animation.channels) {
        vkglTF::AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
        if (sampler.inputs.size() > sampler.outputsVec4.size()) {
            continue;
        }

        for (auto i = 0; i < sampler.inputs.size() - 1; i++) {
            if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
                float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                if (u <= 1.0f) {
                    switch (channel.path) {
                        case vkglTF::AnimationChannel::PathType::TRANSLATION: {
                            glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                            channel.node->translation = glm::vec3(trans);
                            break;
                        }
                        case vkglTF::AnimationChannel::PathType::SCALE: {
                            glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                            channel.node->scale = glm::vec3(trans);
                            break;
                        }
                        case vkglTF::AnimationChannel::PathType::ROTATION: {
                            glm::quat q1;
                            q1.x = sampler.outputsVec4[i].x;
                            q1.y = sampler.outputsVec4[i].y;
                            q1.z = sampler.outputsVec4[i].z;
                            q1.w = sampler.outputsVec4[i].w;
                            glm::quat q2;
                            q2.x = sampler.outputsVec4[i + 1].x;
                            q2.y = sampler.outputsVec4[i + 1].y;
                            q2.z = sampler.outputsVec4[i + 1].z;
                            q2.w = sampler.outputsVec4[i + 1].w;
                            channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
                            break;
                        }
                    }
                    updated = true;
                }
            }
        }
    }
    if (updated) {
        for (auto &node : nodes) {
            node->update();
        }
    }
}

/*
    Helper functions
*/
vkglTF::Node *vkglTF::Model::findNode(Node *parent, uint32_t index)
{
    Node *nodeFound = nullptr;
    if (parent->index == index) {
        return parent;
    }
    for (auto &child : parent->children) {
        nodeFound = findNode(child, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}

vkglTF::Node *vkglTF::Model::nodeFromIndex(uint32_t index)
{
    Node *nodeFound = nullptr;
    for (auto &node : nodes) {
        nodeFound = findNode(node, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}

void vkglTF::Model::prepareNodeDescriptor(vkglTF::Node *node, VkDescriptorSetLayout descriptorSetLayout)
{
    if (node->mesh) {
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool = descriptorPool;
        descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo,
                                                 &node->mesh->uniformBuffer.descriptorSet));

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

        vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
    }
    for (auto &child : node->children) {
        prepareNodeDescriptor(child, descriptorSetLayout);
    }
}

void vkglTF::Model::setDrawMeshNames(std::unordered_set<std::string> meshNames)
{
    if (meshNames.find("all") != meshNames.end()) {
        // show all nodes
        drawMeshNames = allMeshNames;
        return;
    }
    std::unordered_set<std::string> updateMeshNames;
    for (auto &nodeName : meshNames) {
        if (allMeshNames.find(nodeName) != allMeshNames.end()) {
            updateMeshNames.insert(nodeName);
        }
    }
    drawMeshNames.clear();
    drawMeshNames = updateMeshNames;
}