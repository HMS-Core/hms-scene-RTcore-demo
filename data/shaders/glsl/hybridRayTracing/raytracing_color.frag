// clang-format off
layout (early_fragment_tests) in;
#define RAY_TRACING
#define FRAGMENT_SHADER
#extension GL_GOOGLE_include_directive : enable
#include "raytracing.glsl"

// clang-format on
void main()
{
    vec3 n = normalize(inNormal);
    vec3 camPos = vec3(ubo.camPos.xyz);
    camPos.y = -camPos.y;
    // Reflection
    vec3 inDirection = inWorldPos.xyz - camPos.xyz;
    vec3 outDirection = normalize(reflect(inDirection, n));

    Ray ray;
    ray.origin = inWorldPos.xyz + 0.01 * outDirection;
    ray.direction = outDirection;
    ray.mint = 0.01f;
    ray.maxt = 10.0f;

    HitInfo hitInfo;
    hitInfo.t = -1.0f;
    traceRay(ray, RAY_FLAG_CLOSEST_HIT, hitInfo);
#ifdef STAT_REFLECT
    atomicAdd(countBuffer[0], 1);
#endif
    if (hitInfo.t >= 0.0f) {
        const uint triangleId = hitInfo.triId;
        const ivec3 triangleIndices =
        ivec3(indexBuffer[triangleId * 3], indexBuffer[triangleId * 3 + 1], indexBuffer[triangleId * 3 + 2]);
        const vec2 hitTriangelUv = vec2(hitInfo.u, hitInfo.v);
        vec4 baseColor = vec4(0.0);
        vec2 drawSampleUV = vec2(0.5);
        vec3 drawPosition = vec3(0.5);
        vec3 drawNormal = vec3(0.5);
        getHitInformation(triangleIndices, hitTriangelUv, baseColor, drawSampleUV, drawPosition, drawNormal);
#ifdef USE_PBR_REFLECT
        outColor = simplePBR(baseColor, drawSampleUV, drawPosition, drawNormal);
#else
        outColor = baseColor;
#endif
        outColor.rgb = pow(outColor.xyz, 1.0 / vec3(2.2));
        outColor.a = BLEND_FACTOR;
    } else {
        discard;
    }
}
