#version 450
layout (early_fragment_tests) in;
#extension GL_GOOGLE_include_directive : enable
#define GENERATERAY_PIPELINE
#define FRAGMENT_SHADER
#include "common.glsl"
const float INFINITE = 10.0f;

layout(location = 0) in vec4 inWorldPosMeshId;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

void main()
{
    const vec3 inWorldPos = inWorldPosMeshId.xyz;
    const uint pixelIndex = uint(gl_FragCoord.y) * uboParams.framebufferWidth + uint(gl_FragCoord.x);
    vec3 n = normalize(inNormal);
    vec3 camPos = vec3(ubo.camPos.xyz);
    camPos.y = -camPos.y;
    // Reflection
    vec3 inDirection = inWorldPos.xyz - camPos.xyz;
    vec3 outDirection = normalize(reflect(inDirection, n));
    generatedRays[pixelIndex].origin = inWorldPos.xyz + 0.01 * outDirection;
    generatedRays[pixelIndex].dir = outDirection;
    generatedRays[pixelIndex].tmin = 0.01f;
    generatedRays[pixelIndex].tmax = INFINITE;

}