#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inUV;

layout(binding = 0) uniform UBO
{
    mat4 projection;
    mat4 model;
}
ubo;

layout(location = 0) out vec3 outUVW;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPos;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outUVW = inPos.xyz;
    gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
    outWorldPos = vec3(ubo.model * vec4(inPos.xyz, 1.0));
    // Normal in world space
    mat3 mNormal = transpose(inverse(mat3(ubo.model)));
    outNormal = mNormal * normalize(inNormal.xyz);
    outNormal = normalize(outNormal);
}
