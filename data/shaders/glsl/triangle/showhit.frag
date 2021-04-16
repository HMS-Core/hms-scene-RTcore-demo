#version 450
layout(set = 0, binding = 0) uniform UBOParams
{
    uint traceRayHeight;
    uint traceRayWidth;
    uint framebufferHeight;
    uint framebufferWidth;
}
uboParams;

struct HitInfor {
    float t;
    uint triId;
    float u;
    float v;
};

layout(set = 0, binding = 1) buffer readonly HitInforBuffer
{
    HitInfor hitBuffer[];
};

struct Vertex {
    vec4 pos;
    vec4 normal;
    vec4 uv;
    vec4 color;
    vec4 joint0;
    vec4 weight0;
    vec4 padding2;
};

layout(set = 0, binding = 2) buffer readonly VertexBuffer
{
    Vertex vertexBuffer[];
};

layout(set = 0, binding = 3) buffer readonly IndexBuffer
{
    uint indexBuffer[];
};

layout(location = 0) out vec4 outColor;

void getHitColor(const ivec3 triangleIndices, const vec2 centerUV, out vec3 hitColor)
{
    vec3 hitColor0 = vec3(vertexBuffer[triangleIndices.x].color.xyz);
    vec3 hitColor1 = vec3(vertexBuffer[triangleIndices.y].color.xyz);
    vec3 hitColor2 = vec3(vertexBuffer[triangleIndices.z].color.xyz);

    hitColor = hitColor0 * (1 - centerUV.x - centerUV.y) + hitColor1 * centerUV.x + hitColor2 * centerUV.y;
}

void main()
{
    const uint hitIndex =
        uint(gl_FragCoord.y / float(uboParams.framebufferHeight) * uboParams.traceRayHeight) * uboParams.traceRayWidth +
        uint(gl_FragCoord.x / float(uboParams.framebufferWidth) * uboParams.traceRayWidth);

    const float isHit = hitBuffer[hitIndex].t;
    const uint triangleId = hitBuffer[hitIndex].triId;
    const ivec3 triangleIndices =
        ivec3(indexBuffer[triangleId * 3], indexBuffer[triangleId * 3 + 1], indexBuffer[triangleId * 3 + 2]);
    vec3 hitColor;
    if (isHit > 0) {
        // draw hit mesh
        const vec2 hitTriangelUv = vec2(hitBuffer[hitIndex].u, hitBuffer[hitIndex].v);
        getHitColor(triangleIndices, hitTriangelUv, hitColor);
        outColor.xyz = hitColor.xyz;
    } else {
        discard;
    }
}
