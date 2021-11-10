/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: buffer head file
 */

#ifndef VULKANEXAMPLES_BUFFERINFOR_H
#define VULKANEXAMPLES_BUFFERINFOR_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace buf {
struct UBOMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    glm::vec3 camPos;
};

struct UBOParams {
    union {
        struct {
            float exposure;
            float gamma;
        };
        struct {
            uint32_t traceRayHeight;
            uint32_t traceRayWidth;
        };
    };

    uint32_t framebufferHeight = 0;
    uint32_t framebufferWidth = 0;
    glm::vec4 lightDir;
    float prefilteredCubeMipLevels;
    float scaleIBLAmbient = 1.0f;
    float debugViewInputs = 0;
    float debugViewEquation = 0;
    int frame = 0;
    int aoSamples = 1;
    float aoRadius = 2.0f;
};

struct PushConstBlockMaterial {
    glm::vec4 baseColorFactor;
    glm::vec4 emissiveFactor;
    glm::vec4 diffuseFactor;
    glm::vec4 specularFactor;
    float workflow;
    int colorTextureSet;
    int PhysicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
    float metallicFactor;
    float roughnessFactor;
    float alphaMask;
    float alphaMaskCutoff;
    uint32_t meshId;
};

struct BVHMesh {
    float *vertices = nullptr;
    uint32_t numVertices;
    uint32_t *indices = nullptr;
    uint32_t numIndices;
    uint32_t stride;
};

struct Ray {
    float origin[3]; /* *< The ray origin, i.e., xyz. */
    float tmin;      /* *< The min distance of potential ray hit. */
    float dir[3];    /* *< The ray direction in the same space of AS. */
    float tmax;      /* *< The max distance of potential ray hit. */
};
} // namespace buf

#endif // VULKANEXAMPLES_BUFFERINFOR_H
