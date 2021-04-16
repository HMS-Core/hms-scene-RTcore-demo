#version 450

layout(set = 0, binding = 2) uniform sampler2D samplertexutre;
layout(location = 0) out vec4 outFragColor;
layout(set = 0, binding = 1) uniform UBOParams
{
    float exposure;
    float gamma;
    uint framebufferHeight;
    uint framebufferWidth;
    vec4 lightDir;
    float prefilteredCubeMipLevels;
    float scaleIBLAmbient;
    float debugViewInputs;
    float debugViewEquation;
}
uboParams;

void main()
{
    vec2 uv =
        vec2(gl_FragCoord.x / float(uboParams.framebufferWidth), gl_FragCoord.y / float(uboParams.framebufferHeight));
    vec3 fragPos = texture(samplertexutre, uv).rgb;
    outFragColor.rgb = fragPos;
}