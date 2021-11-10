// clang-format off
#version 450

layout(early_fragment_tests) in;

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

layout(set = 0, binding = 2) uniform sampler2D samplertexutre;

layout(location = 0) out vec4 outFragColor;

void main()
{
    vec2 uv =
        vec2(gl_FragCoord.x / float(uboParams.framebufferWidth), gl_FragCoord.y / float(uboParams.framebufferHeight));
    outFragColor = texture(samplertexutre, uv);
}
