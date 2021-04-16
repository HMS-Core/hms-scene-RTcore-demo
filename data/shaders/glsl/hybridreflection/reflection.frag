// PBR shader based on the Khronos WebGL PBR implementation
// See https://github.com/KhronosGroup/glTF-WebGL-PBR
// Supports both metallic roughness and specular glossiness inputs

#version 450
#extension GL_GOOGLE_include_directive : enable
#define REFLECTION_PIPELINE
#define FRAGMENT_SHADER
#include "common.glsl"
layout(location = 0) in vec4 inWorldPosMeshId;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void getHitInformation(const ivec3 triangleIndices, const vec2 centerUV, out vec2 hitSampeUV, out vec3 hitPosition,
                       out vec3 hitNormal)
{
    vec2 hitSampleUV0 = vertexBuffer[triangleIndices.x].uv.xy;
    vec2 hitSampleUV1 = vertexBuffer[triangleIndices.y].uv.xy;
    vec2 hitSampleUV2 = vertexBuffer[triangleIndices.z].uv.xy;

    vec3 hitPosition0 = vec3(vertexBuffer[triangleIndices.x].pos.xyz);
    vec3 hitPosition1 = vec3(vertexBuffer[triangleIndices.y].pos.xyz);
    vec3 hitPosition2 = vec3(vertexBuffer[triangleIndices.z].pos.xyz);

    vec3 hitNormal0 = vec3(vertexBuffer[triangleIndices.x].normal.xyz);
    vec3 hitNormal1 = vec3(vertexBuffer[triangleIndices.y].normal.xyz);
    vec3 hitNormal2 = vec3(vertexBuffer[triangleIndices.z].normal.xyz);

    vec4 joint0_0 = vertexBuffer[triangleIndices.x].joint0;
    vec4 joint0_1 = vertexBuffer[triangleIndices.y].joint0;
    vec4 joint0_2 = vertexBuffer[triangleIndices.z].joint0;

    vec4 weight0_0 = vertexBuffer[triangleIndices.x].weight0;
    vec4 weight0_1 = vertexBuffer[triangleIndices.y].weight0;
    vec4 weight0_2 = vertexBuffer[triangleIndices.z].weight0;

    convetLocalToWorld(hitPosition0, hitNormal0, joint0_0, weight0_0);
    convetLocalToWorld(hitPosition1, hitNormal1, joint0_1, weight0_1);
    convetLocalToWorld(hitPosition2, hitNormal2, joint0_2, weight0_2);

    hitSampeUV = hitSampleUV0 * (1 - centerUV.x - centerUV.y) + hitSampleUV1 * centerUV.x + hitSampleUV2 * centerUV.y;
    hitPosition = hitPosition0 * (1 - centerUV.x - centerUV.y) + hitPosition1 * centerUV.x + hitPosition2 * centerUV.y;
    hitNormal = hitNormal0 * (1 - centerUV.x - centerUV.y) + hitNormal1 * centerUV.x + hitNormal2 * centerUV.y;
}

void main()
{
    const uint currentMeshId = uint(inWorldPosMeshId.w);
    const uint pixelIndex = uint(gl_FragCoord.y) * uboParams.framebufferWidth + uint(gl_FragCoord.x);
    const float isHit = hitBuffer[pixelIndex].t;
    const uint triangleId = hitBuffer[pixelIndex].triId;
    const ivec3 triangleIndices =
        ivec3(indexBuffer[triangleId * 3], indexBuffer[triangleId * 3 + 1], indexBuffer[triangleId * 3 + 2]);
    // mesh id is stored in w of postion;
    const uint hitMeshId = uint(vertexBuffer[triangleIndices.x].pos.w);
    vec2 drawSampleUV;
    vec3 drawPosition;
    vec3 drawNormal;

    if (isHit < 0 && currentMeshId == material.meshId) {
        // Draw relection mesh
        drawSampleUV = inUV;
        drawPosition = inWorldPosMeshId.xyz;
        drawNormal = inNormal.xyz;
    } else if (isHit > 0 && hitMeshId == material.meshId) {
        // draw hit mesh
        const vec2 hitTriangelUv = vec2(hitBuffer[pixelIndex].u, hitBuffer[pixelIndex].v);
        getHitInformation(triangleIndices, hitTriangelUv, drawSampleUV, drawPosition, drawNormal);
    } else {
        discard;
        return;
    }

    outColor = pbrRendering(drawSampleUV, drawPosition, drawNormal, false);
    outColor.xyz = pow(outColor.xyz, 1.0 / vec3(2.2));
}
