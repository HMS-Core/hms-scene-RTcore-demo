// clang-format off
#ifndef RAY_TRACING
layout(set = 0, binding = 0) uniform UBO {
    mat4 projection;
    mat4 model;
    mat4 view;
    vec3 camPos;
} ubo;

layout(set = 0, binding = 1) uniform UBOParams {
    float exposure;
    float gamma;
    uint framebufferHeight;
    uint framebufferWidth;
    vec4 lightDir;
    float prefilteredCubeMipLevels;
    float scaleIBLAmbient;
    float debugViewInputs;
    float debugViewEquation;
} uboParams;
#else
layout(set = 0, binding = 4) uniform UBO {
    mat4 projection;
    mat4 model;
    mat4 view;
    vec3 camPos;
} ubo;

layout(set = 0, binding = 5) uniform UBOParams {
    float exposure;
    float gamma;
    uint framebufferHeight;
    uint framebufferWidth;
    vec4 lightDir;
    float prefilteredCubeMipLevels;
    float scaleIBLAmbient;
    float debugViewInputs;
    float debugViewEquation;
} uboParams;

#ifdef FRAGMENT_SHADER
struct Vertex {
    vec4 pos;
    vec4 normal;
    vec4 uv;
    vec4 color;
    vec4 joint0;
    vec4 weight0;
    vec4 padding2;
};
layout(set = 0, binding = 6) buffer readonly VertexBuffer {
    Vertex vertexBuffer[];
};

layout(set = 0, binding = 7) buffer readonly IndexBuffer {
    uint indexBuffer[];
};

layout(set = 0, binding = 8) buffer CountBuffer {
    uint countBuffer[];
};
#endif // FRAGMENT_SHADER

#endif // RAY_TRACING

//  Set 1: used for ibl, generated in VulkanImageBasedLighting pipeline
//  2 cube textures
//  1 2D texture
layout(set = 1, binding = 0) uniform samplerCube samplerIrradiance;
layout(set = 1, binding = 1) uniform samplerCube prefilteredMap;
layout(set = 1, binding = 2) uniform sampler2D samplerBRDFLUT;


// Set 2: binding material textures which are loaded in VulkanglTFModel
// 5 2d textures
layout(set = 2, binding = 0) uniform sampler2D colorMap;
layout(set = 2, binding = 1) uniform sampler2D physicalDescriptorMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;
layout(set = 2, binding = 3) uniform sampler2D aoMap;
layout(set = 2, binding = 4) uniform sampler2D emissiveMap;

// Set 3: binding each node's local and joint matrices, which are loaded in VulkanglTFModel
// 1 uniform buffer
#define MAX_NUM_JOINTS 128
layout(set = 3, binding = 0) uniform UBONode {
    mat4 matrix;
    mat4 jointMatrix[MAX_NUM_JOINTS];
    float jointCount;
} node;


// Push constant
layout(push_constant) uniform Material {
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    vec4 diffuseFactor;
    vec4 specularFactor;
    float workflow;
    int baseColorTextureSet;
    int physicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
    float metallicFactor;
    float roughnessFactor;
    float alphaMask;
    float alphaMaskCutoff;
    uint meshId;
} material;

void convetLocalToWorld(inout vec3 inOutPos, inout vec3 inOutNormal, const vec4 joint0, const vec4 weight0)
{
    vec4 locPos;
    vec3 outWorldPos;
    vec3 outNormal;
    if (node.jointCount > 0.0) {
        // Mesh is skinned
        mat4 skinMat = weight0.x * node.jointMatrix[int(joint0.x)] + weight0.y * node.jointMatrix[int(joint0.y)] +
                       weight0.z * node.jointMatrix[int(joint0.z)] + weight0.w * node.jointMatrix[int(joint0.w)];

        locPos = ubo.model * node.matrix * skinMat * vec4(inOutPos.xyz, 1.0);
        outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix * skinMat))) * inOutNormal.xyz);
    } else {
        locPos = ubo.model * node.matrix * vec4(inOutPos.xyz, 1.0);
        outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix))) * inOutNormal.xyz);
    }
    locPos.y = -locPos.y;
    outWorldPos = locPos.xyz / locPos.w;

    outNormal.y *= -1.0f;

    inOutPos = outWorldPos;
    inOutNormal = outNormal;
}

#ifdef FRAGMENT_SHADER
// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal(const vec2 uv, in vec3 worlPosition, in vec3 worldNormal)
{
    // Perturb normal, see http://www.thetenthplanet.de/archives/1180
    vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;

    vec3 q1 = dFdx(worlPosition);
    vec3 q2 = dFdy(worlPosition);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N = normalize(worldNormal);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo {
    float NdotL;               // cos angle between normal and light direction
    float NdotV;               // cos angle between normal and view direction
    float NdotH;               // cos angle between normal and half vector
    float LdotH;               // cos angle between light direction and half vector
    float VdotH;               // cos angle between view direction and half vector
    float perceptualRoughness; // roughness value, as authored by the model creator (input to shader)
    float metalness;           // metallic value at the surface
    vec3 reflectance0;         // full reflectance color (normal incidence angle)
    vec3 reflectance90;        // reflectance color at grazing angle
    float alphaRoughness;      // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;         // color contribution from diffuse lighting
    vec3 specularColor;        // color contribution from specular lighting
};

const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 1.0f;

#define MANUAL_SRGB 1

vec3 Uncharted2Tonemap(vec3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

vec4 tonemap(vec4 color)
{
    vec3 outcol = Uncharted2Tonemap(color.rgb * uboParams.exposure);
    outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    return vec4(pow(outcol, vec3(1.0f / uboParams.gamma)), color.a);
}

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
#ifdef MANUAL_SRGB
#ifdef SRGB_FAST_APPROXIMATION
    vec3 linOut = pow(srgbIn.xyz, vec3(2.2));
#else  // SRGB_FAST_APPROXIMATION
    vec3 bLess = step(vec3(0.04045), srgbIn.xyz);
    vec3 linOut = mix(srgbIn.xyz / vec3(12.92), pow((srgbIn.xyz + vec3(0.055)) / vec3(1.055), vec3(2.4)), bLess);
#endif // SRGB_FAST_APPROXIMATION
    return vec4(linOut, srgbIn.w);
    ;
#else  // MANUAL_SRGB
    return srgbIn;
#endif // MANUAL_SRGB
}

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)
{
    float lod = (pbrInputs.perceptualRoughness * uboParams.prefilteredCubeMipLevels);
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec3 brdf = (texture(samplerBRDFLUT, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness))).rgb;
    vec3 diffuseLight = SRGBtoLINEAR(tonemap(texture(samplerIrradiance, n))).rgb;

    vec3 specularLight = SRGBtoLINEAR(tonemap(textureLod(prefilteredMap, reflection, lod))).rgb;

    vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
    vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

    // For presentation, this allows us to disable IBL terms
    // For presentation, this allows us to disable IBL terms
    diffuse *= uboParams.scaleIBLAmbient;
    specular *= uboParams.scaleIBLAmbient;

    return diffuse + specular;
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
    return pbrInputs.reflectance0 +
           (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S.
// Trowbridge, and K. P. Reitz Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC
// Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (M_PI * f * f);
}

// Gets metallic factor from specular glossiness workflow inputs
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
    float perceivedDiffuse =
        sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
    float perceivedSpecular =
        sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
    if (perceivedSpecular < c_MinRoughness) {
        return 0.0;
    }
    float a = c_MinRoughness;
    float b =
        perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinRoughness) + perceivedSpecular - 2.0 * c_MinRoughness;
    float c = c_MinRoughness - perceivedSpecular;
    float D = max(b * b - 4.0 * a * c, 0.0);
    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

vec4 pbrRendering(const vec2 drawSampleUV, const vec3 drawPosition, const vec3 drawNormal, bool useNormalTexture)
{
    float perceptualRoughness;
    float metallic;
    vec3 diffuseColor;
    vec4 baseColor;

    vec3 f0 = vec3(0.04);
    if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {
        // Metallic and Roughness material properties are packed together
        // In glTF, these factors can be specified by fixed scalar values
        // or from a metallic-roughness map
        perceptualRoughness = material.roughnessFactor;
        metallic = material.metallicFactor;
        if (material.physicalDescriptorTextureSet > -1) {
            // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
            // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
            vec4 mrSample = texture(physicalDescriptorMap, drawSampleUV);
            perceptualRoughness = mrSample.g * perceptualRoughness;
            metallic = mrSample.b * metallic;
        } else {
            perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
            metallic = clamp(metallic, 0.0, 1.0);
        }
        // Roughness is authored as perceptual roughness; as is convention,
        // convert to material roughness by squaring the perceptual roughness [2].

        // The albedo may be defined from a base texture or a flat color
        if (material.baseColorTextureSet > -1) {
            baseColor = SRGBtoLINEAR(texture(colorMap, drawSampleUV)) * material.baseColorFactor;
        } else {
            baseColor = material.baseColorFactor;
        }
    }

    diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
    diffuseColor *= 1.0 - metallic;

    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    vec3 specularColor = mix(f0, baseColor.rgb, metallic);

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical
    // fresnel effect. For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing
    // reflecance to 0%.
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specularEnvironmentR0 = specularColor.rgb;
    vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;
    vec3 n;
    if (useNormalTexture) {
        n = (material.normalTextureSet > -1) ? getNormal(drawSampleUV, drawPosition, drawNormal)
                                             : normalize(drawNormal);
    } else {
        n = normalize(drawNormal);
    }

    vec3 v = normalize(ubo.camPos - drawPosition); // Vector from surface point to camera
    vec3 l = normalize(uboParams.lightDir.xyz);    // Vector from surface point to light
    vec3 h = normalize(l + v);                     // Half vector between both l and v
    vec3 reflection = -normalize(reflect(v, n));
    reflection.y *= -1.0f;

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(NdotL, NdotV, NdotH, LdotH, VdotH, perceptualRoughness, metallic, specularEnvironmentR0,
                                specularEnvironmentR90, alphaRoughness, diffuseColor, specularColor);

    // Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

    const vec3 u_LightColor = vec3(1.0);

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    vec3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);

    // Calculate lighting contribution from image based lighting source (IBL)
    color += getIBLContribution(pbrInputs, n, reflection);

    const float u_OcclusionStrength = 1.0f;
    // Apply optional PBR terms for additional (optional) shading
    if (material.occlusionTextureSet > -1) {
        float ao = texture(aoMap, drawSampleUV).r;
        color = mix(color, color * ao, u_OcclusionStrength);
    }

    const float u_EmissiveFactor = 1.0f;
    if (material.emissiveTextureSet > -1) {
        vec3 emissive = SRGBtoLINEAR(texture(emissiveMap, drawSampleUV)).rgb * u_EmissiveFactor;
        color += emissive;
    }

    return vec4(color, baseColor.a);
}

#endif // end of FRAGMENT_SHADER
