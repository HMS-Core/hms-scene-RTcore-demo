/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: RayShop head file
 */

#ifndef TRAVERSAL_H
#define TRAVERSAL_H

/**
* @file  Travsersal.h
* @brief The building, intersection and other ops of rayshop.
* @author   Huawei
* @date     2020-8-11
* @version  1.0
* @par Copyright (c):
*/

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

namespace RayShop {
using BLAS = uint32_t;

/// @brief The flags used in tracing rays.
enum TraceRayFlag {
    /* *< The default triangle intersection algorithm. */
    TRACERAY_FLAG_INTERSECT_DEFAULT           = 0,
    /* *< Return immediately at the first hit. Don't mix it with ANYHIT in DXR spec. */
    TRACERAY_FLAG_ANY_HIT                     = 0x1,
    /* *< Find the closest hit. This is exclusive to any hit flag. */
    TRACERAY_FLAG_CLOSEST_HIT                 = 0x2,
    /* *< Don't hit the back face of the geometry. The front face is defined as counter-clockwise. */
    TRACERAY_FLAG_CULL_BACK_FACING_TRIANGLES  = 0x4,
    /* *< Ditto. It is exclusive to back face culling flag. */
    TRACERAY_FLAG_CULL_FRONT_FACING_TRIANGLES = 0x8,
};

/// @brief The data format hit result.
enum class TraceRayHitFormat {
    /* *< ray distance (t<0 for miss) */
    T,
    /* *< ray distance (t<0 for miss), primitive id(the triangle index of the mesh). */
    T_PRIMID,
    /* *< ray distance (t<0 for miss), primitive id, barycentric u,v (in the hit triangle space w.r.t.
     * the mesh vertex order). */
    T_PRIMID_U_V,
    /* *< ray distance (t<0 for miss), primitive id, instance id (the tlas's instance index by insertion order). */
    T_PRIMID_INSTID,
    /* *< ray distance (t<0 for miss), primitive id, instance id, and the barycentric u,v. */
    T_PRIMID_INSTID_U_V,
};

/// @brief ray structure. The users should generate rays in this format and pack them into a buffer.
struct Ray {
    float origin[3];     /* *< The vector 3 of ray origin, i.e., xyz. */
    float tmin;          /* *< The min distance of potential ray hit. */
    float dir[3];        /* *< The vector 3 of ray direction in the same space of AS. */
    float tmax;          /* *< The max distance of potential ray hit. */
};

/// @brief The data definition for TraceRayHitFormat::T format
struct HitDistance {
    float t;
};

/// @brief The data definition for TraceRayHitFormat::T_PRIMID format
struct HitDistancePrimitive {
    float t;
    uint32_t primId;
};

/// @brief The data definition for TraceRayHitFormat::T_PRIMID_U_V format
struct HitDistancePrimitiveCoordinates {
    float t;
    uint32_t primId;
    float u;
    float v;
};

/// @brief The data definition for TraceRayHitFormat::T_PRIMID_INSTID format
struct HitDistancePrimitiveInstance {
    float t;
    uint32_t primId;
    uint32_t instId;
};

/// @brief The data definition for TraceRayHitFormat::T_PRIMID_INSTID_U_V format
struct HitDistancePrimitiveInstanceCoordinates {
    float t;
    uint32_t primId;
    uint32_t instId;
    float u;
    float v;
};

/// @brief Acceleration structure build method. Each method has cons and pros. Users
/// should choose the appropriate one due to the use scenario.
enum class ASBuildMethod {
    SAH_CPU = 0,                        /* *< Best quality and slow building by cpu. */
    SAH_GPU = 1,                        /* *< Best quality and slow building by gpu. */
};

/// @brief data source
enum class BufferType {
    CPU = 0,                        /* *< Data on CPU */
    GPU                             /* *< Data on GPU */
};

/// @brief The buffer object which is used by travsersal process.
/// If gpu VkBuffer is used, It should set VK_BUFFER_USAGE_TRANSFER_SRC_BIT.
struct Buffer {
    BufferType type;                /* *< The location of the this buffer. */
    union {
        void *cpuBuffer;            /* *< Cpu buffer if type is CPU. */
        VkBuffer gpuVkBuffer;       /* *< VkBuffer if type is GPU on Vulkan. */
        void *other;                /* *< Other buffer type. */
    };
};

/// @brief The geometry description, typically a triangular mesh, w.r.t a bottom level acceleration structure.
struct GeometryTriangleDescription {
    Buffer vertice;                 /* *< The vertices buffer. */
    uint32_t stride;                /* *< The size of each vertex in 4 bytes, the first 3 floats must be position. */
    uint32_t verticeCount;          /* *< The vertices count. */

    Buffer indice;                  /**< The indices buffer. */
    uint32_t indiceCount;           /**< The indices count. */
};

/// @brief The instance description. An instance refers to a blas with an affine transformation.
struct InstanceDescription {
    float transform[4][4];          /* *< The 4x4 affine transform matrix, e.g., rotation, scaling and translation. */
    BLAS blas;                      /* *< The referred blas. */
};

/// @brief The error code.
enum class Result {
    SUCCESS = 0,                    /* *< Success. */
    NOT_READY,                      /* *< Vulkan device is not available. */
    INVALID_PARAMETER,              /* *< Invalid parameter. */
    OUT_OF_MEMORY,                  /* *< Not enough memory. */
    UNKNOWN_ERROR,                  /* *< Other unknown errors. */
};

/// @brief The mesh node description.
struct Node {
    uint32_t firstIndex;            /* *< The node of first index */
    uint32_t indicesCount;          /* *< The node of index count */
    Buffer modelMat;                /* *< The node of model transform matrix buffer handle */
};

/// @brief The mesh description.
struct Mesh {
    Buffer vertex;                  /* *< The mesh vertex buffer */
    Buffer normal;                  /* *< The mesh normal buffer */
    Buffer index;                   /* *< The mesh index buffer */
    std::vector<Node> nodes;        /* *< The mesh node description */
};

/// @brief The Camera description.
struct Camera {
    float vpMat[4][4];               /* *< The mat4x4 view * project matrix of camera */
    float pos[4];                    /* *< The vector 4 of camera position */
};

/// @brief The Size description.
struct Size {
    uint32_t width;                  /* *< The width of size */
    uint32_t height;                 /* *< The height of size */
};

/// @brief The rays generated by mesh description.
struct RaysMeshDescription {
    Mesh mesh;                      /* *< The reflective mesh description */
    TraceRayFlag rayFlag;           /* *< The traversal flag */
    TraceRayHitFormat hitFormat;    /* *< The hit buffer format */
    Camera camera;                  /* *< The ray generated by camera info */
    Size region;                    /* *< The ray generated region size */
};

/// @brief The VkContext description.
struct VkContext {
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;     /* *< vulkan command buffer created by user */
    VkRenderPass renderpass = VK_NULL_HANDLE;       /* *< renderpass created by user */
};

class TraversalImpl;

class Traversal {
public:
    /**
     * Default constructor.
     */
    explicit Traversal();

    /**
     * Destructor.
     */
    ~Traversal();
#ifdef __ANDROID__
    /**
     * Initialize the traversal context and allocate some resources for internal use.
     * @param[in]   physicalDevice          The Vulkan physical device.
     * @param[in]   device                  The logical Vulkan device.
     * @param[in]   computeQueue            The compute command queue that raytracing commands will be submitted to.
     * @param[in]   computeIndice           The compute queue indice.
     * @param[in]   env                     The pointer that encapsulates JNI methods.
     * @return      Result                  Check out error code. @see Result
     */
    Result Setup(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue computeQueue,
                 uint32_t computeIndices, JNIEnv *env) const noexcept;
#endif
    /*
     * Initialize the traversal context and allocate some resources for internal use.
     * @param[in]   physicalDevice          The Vulkan physical device.
     * @param[in]   device                  The logical Vulkan device.
     * @param[in]   computeQueue            The compute command queue that raytracing commands will be submitted to.
     * @param[in]   computeIndice           The compute queue indice.
     * @return      Result                  Check out error code. @see Result
     */
    Result Setup(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue computeQueue,
                 uint32_t computeIndices) const noexcept;

    /*
     * Release internal resources.
     */
    void Destroy() const noexcept;

    /**
     * Create the bottom level acceleration structure.
     * @param[in]   method                  The build method.
     * @param[in]   geometriesCount         The number of geometries, e.g., the triangle mesh count.
     * @param[in]   *geometries             An array of geometries.
     * @param[out]  *blases                 An array of output bottom level acceleration structures,
     *                                      each of which corresponds to a geometry object.
     * @return      Result                  Check out error code. @see Result
     * @note
     */
    Result CreateBLAS(ASBuildMethod                       method,
                      uint32_t                            geometriesCount,
                      const GeometryTriangleDescription   *geometries,
                      BLAS                                *blases) const noexcept;

    /**
     * Create the top level acceleration structure from a bunch of BLASes.
     * @param[in]   instancesCount      The number of instances.
     * @param[in]   *instances          An array of instance descriptions.
     * @return      Result              Check out error code. @see Result
     * @note
     */
    Result CreateTLAS(uint32_t                    instancesCount,
                      const InstanceDescription   *instances) const noexcept;

    /**
     * Refit the bottom level acceleration structure. It needs to be called when the geometry of
     * the blas changes but users don't want to rebuild it entirely. Note that
     * a refit AS' quality degrades. In every other a few frames, BLAS needs to be rebuilt anyway.
     * @param[in]   geometriesCount         The number of geometries, e.g., triangular mesh count.
     * @param[in]   *geometries             An array of geometries.
     * @param[in]   *blases                 An array of bottom level acceleration structures,
     *                                      each of which corresponds to a geometry object.
     * @param[in]   cmdBuffer               vulkan command buffer created by user
     * @return      Result                  Check out error code. @see Result
     * @note
     */
    Result RefitBLAS(uint32_t                             geometriesCount,
                     const GeometryTriangleDescription    *geometries,
                     const BLAS                           *blases,
                     VkCommandBuffer cmdBuffer = VK_NULL_HANDLE) const noexcept;

    /**
     * Destroy the bottom level acceleration structure. We recommend users explicitly destroy blas
     * when the blas is not in use anymore. An unreferenced blas will still occupy memory and drag
     * down the system performance in some cases. All blases will be destroyed when the traversal
     * object destroys itself anyhow.
     * @param[in]   instancesCount      The number of instances.
     * @param[in]   blas                Bottom level acceleration structure.
     * @return      Result              Check out error code. @see Result
     * @note
     */
    Result DestroyBLAS(uint32_t geometriesCount, const uint32_t *blas) const noexcept;

    /**
     * Trace a given number of rays and return the hit results of each ray.
     * @param[in]   rayCount            The ray number.
     * @param[in]   rayFlags            The traversal flag, combination of any hit/closest hit,
     *                                  backface culling/frontface culling
     * @param[in]   rays                The ray buffer
     * @param[in]   hits                The hit buffer, the buffer should allocate before call TraceRays(),
     *                                  it's size should be rayCount * hitFormatStride
     * @param[in]   hitFormat           The hit buffer format
     * @return      Result              Check out error code. @see Result
     * @see
     * @note
     */
    Result TraceRays(uint32_t rayCount, uint32_t rayFlags, Buffer rays, Buffer hits,
                     TraceRayHitFormat hitFormat, VkCommandBuffer cmdBuf = VK_NULL_HANDLE) const noexcept;

    /**
    * Trace refected rays on gived reflective mesh on graphic pipeline
    * @param[in]   rayMesh             The ray generated by mesh info, ray only suport TraceRayFlag
    *                                  TRACERAY_FLAG_CLOSEST_HIT and TraceRayHitFormat PRIMID_INSTID
    * @param[out]  hits                The hit buffer, the buffer should allocate before call TraceRays(),
    *                                  it's size should be (the width of rayMesh region + 1) *
    *                                  (the height of rayMesh region + 1) * hitFormatStride
    * @param[in]   vkContext           The vulkan current context including command buffer handle for traceRay pipeline
    *                                  and current active renderpass
    * @return      Result              Check out error code. @see Result
    * @see
    * @note
    */
    Result TraceRays(const RaysMeshDescription &rayMesh,
                     const std::vector<Buffer> &hits,
                     VkContext vkContext) const noexcept;

    /**
     * The get the size in bytes of a hit buffer record.
     * @param[in]   hitFormat           A specific hit buffer format, @see TraceRayHitFormat.
     * @return      uint32_t            The hit format record size.
     * @note
     */
    static uint32_t GetHitFormatBytes(TraceRayHitFormat hitFormat) noexcept;

    /**
     * Get text explanation of an error code.
     * @param[in]   err                 The error code, @see Result.
     * @return      const char *        The text value of an error code.
     */
    static const char *GetErrorCodeString(Result err) noexcept;

    Traversal(const Traversal&) = delete;
    Traversal& operator=(const Traversal&) = delete;
    Traversal(Traversal&&) = delete;
    Traversal& operator=(Traversal&&) = delete;

private:
    std::unique_ptr<TraversalImpl> m_impl;
};
}; // namespace RayShop

#endif // TRAVERSAL_H
