#include <metal_stdlib>
using namespace metal;

struct Fragment_Shader
{
    struct Uniforms_UniformDOF
    {
        float maxRadius;
        float blend;
    };
    constant Uniforms_UniformDOF& UniformDOF;
    struct VSOutput
    {
        float2 UV;
    };
    sampler samplerLinear;
    sampler samplerPoint;
    texture2d<float> TextureCoC;
    texture2d<float> TextureColor;
    const float GOLDEN_ANGLE = 2.39996316;
    const float MAX_BLUR_SIZE = 20.0;
    const float RAD_SCALE = float(1.5);
    float getBlurSize(float2 coc)
    {
        if (coc.r > 0.0)
        {
            return ((((coc).r * UniformDOF.maxRadius) * MAX_BLUR_SIZE) * float(5));
        }
        return ((((coc).g * UniformDOF.maxRadius) * MAX_BLUR_SIZE) * float(5));
    };
    float4 main(VSOutput input)
    {
        uint w, h;
        w = TextureColor.get_width();
        h = TextureColor.get_height();
        float2 pixelSize = (float2(1.0) / float2((float)w, (float)h));
        float3 color = (TextureColor.sample(samplerLinear, (input).UV)).rgb;
        float2 coc = (TextureCoC.sample(samplerPoint, (input).UV)).rg;
        float centerSize = getBlurSize(coc);
        float total = float(1.0);
        float radius = RAD_SCALE;
        for (float ang = float(0.0); (radius < MAX_BLUR_SIZE); (ang += GOLDEN_ANGLE))
        {
            float2 tc = ((input).UV + ((float2(cos(ang), sin(ang)) * pixelSize) * float2(radius)));
            float3 sampleColor = (TextureColor.sample(samplerLinear, tc)).rgb;
            float3 sampleCoC = (TextureCoC.sample(samplerPoint, tc)).rgb;
            float sampleSize = getBlurSize((sampleCoC).xy);
            float m = smoothstep((radius - 0.5), (radius + 0.5), sampleSize);
            (color += mix((color / float3(total)), sampleColor, float3(m)));
            (total += float(1.0));
            (radius += (RAD_SCALE / radius));
        }
        return float4((color / float3(total)), 1.0);
    };

    Fragment_Shader(constant Uniforms_UniformDOF& UniformDOF, sampler samplerLinear, sampler samplerPoint, texture2d<float> TextureCoC, texture2d<float> TextureColor) :
        UniformDOF(UniformDOF), samplerLinear(samplerLinear), samplerPoint(samplerPoint), TextureCoC(TextureCoC), TextureColor(TextureColor)
    {}
};

struct main_input
{
    float2 TEXCOORD0  [[user(locn0)]];
};

struct ArgBuffer0
{
    sampler samplerLinear [[id(0)]];
    sampler samplerPoint [[id(1)]];
};

struct ArgBuffer1
{
    texture2d<float> TextureCoC [[id(0)]];
    texture2d<float> TextureColor [[id(1)]];
    constant Fragment_Shader::Uniforms_UniformDOF& UniformDOF [[id(2)]];
};

fragment float4 stageMain(
	main_input inputData [[stage_in]],
    constant ArgBuffer0& argBuffer0 [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgBuffer1& argBuffer1 [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    Fragment_Shader::VSOutput input0;
    input0.UV = inputData.TEXCOORD0;
    Fragment_Shader main(argBuffer1.UniformDOF, argBuffer0.samplerLinear, argBuffer0.samplerPoint, argBuffer1.TextureCoC, argBuffer1.TextureColor);
    return main.main(input0);
}
