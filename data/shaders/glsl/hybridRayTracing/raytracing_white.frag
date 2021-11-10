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
    if (hitInfo.t >= 0.0f) {
            outColor = vec4(0.96, 0.93, 0.88, 0.7);
    } else {
        discard;

    }
}