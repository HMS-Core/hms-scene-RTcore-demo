// PBR shader based on the Khronos WebGL PBR implementation
// See https://github.com/KhronosGroup/glTF-WebGL-PBR
// Supports both metallic roughness and specular glossiness inputs

#version 450
#extension GL_GOOGLE_include_directive : enable
#define FRAGMENT_SHADER
#include "common.glsl"
layout(location = 0) in vec4 inWorldPosMeshId;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
    const vec3 inWorldPos = inWorldPosMeshId.xyz;

    outColor = pbrRendering(inUV, inWorldPos, inNormal, true);

    outColor.xyz = pow(outColor.xyz, 1.0 / vec3(2.2));
}
