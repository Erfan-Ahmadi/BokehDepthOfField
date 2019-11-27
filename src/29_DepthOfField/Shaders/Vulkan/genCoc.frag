#version 450 core

layout(location = 0) in vec2 UV;

layout(location = 0) out vec2 OutCoC; 

layout(binding = 0) uniform sampler samplerLinear;
layout(binding = 1) uniform sampler samplerPoint;
layout(UPDATE_FREQ_PER_FRAME, binding = 1) uniform texture2D DepthTexture;

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform UniformDOF
{
    float maxRadius;
    float blend;
    float nb;
    float ne;
    float fb;
    float fe;
    vec2 projParams;
};

float LinearizeDepth(float depth)
{
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

void main()
{
    float depth = texture(sampler2D(DepthTexture, samplerPoint), UV).r;
    float depth_linearized = LinearizeDepth(depth);
    float r = ((ne - depth_linearized) / (ne - nb));
    float g = ((depth_linearized - fb) / (fe - fb));
    OutCoC = vec2(r, g);
}
