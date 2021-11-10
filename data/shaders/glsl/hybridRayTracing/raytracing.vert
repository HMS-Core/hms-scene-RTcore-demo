#version 450

#define RAY_TRACING
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inUV;
layout(location = 3) in vec4 inJoint0;
layout(location = 4) in vec4 inWeight0;

layout(location = 0) out vec4 outWorldPosMeshId;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vec3 outWorldPos = inPos.xyz;
    outNormal = inNormal.xyz;
    convetLocalToWorld(outWorldPos, outNormal, inJoint0, inWeight0);
    outUV = inUV.xy;
    gl_Position = ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
    outWorldPosMeshId = vec4(outWorldPos, inPos.w);
}
