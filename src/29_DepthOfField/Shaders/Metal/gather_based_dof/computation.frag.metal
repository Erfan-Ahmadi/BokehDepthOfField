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
    struct PSOut
    {
        float4 TextureNear;
        float4 TextureFar;
    };
    const array<float2, 48> offsets = { {(float2(2.0) * float2(1.0, 0.0)), (float2(2.0) * float2(0.70710704, 0.70710704)), (float2(2.0) * float2((-0.0), 1.0)), (float2(2.0) * float2((-0.70710704), 0.70710704)), (float2(2.0) * float2((-1.0), (-0.0))), (float2(2.0) * float2((-0.707106), (-0.70710704))), (float2(2.0) * float2(0.0, (-1.0))), (float2(2.0) * float2(0.70710704, (-0.70710704))), (float2(4.0) * float2(1.0, 0.0)), (float2(4.0) * float2(0.92388, 0.38268300)), (float2(4.0) * float2(0.70710704, 0.70710704)), (float2(4.0) * float2(0.38268300, 0.92388)), (float2(4.0) * float2((-0.0), 1.0)), (float2(4.0) * float2((-0.382684), 0.9238790)), (float2(4.0) * float2((-0.70710704), 0.70710704)), (float2(4.0) * float2((-0.92388), 0.38268300)), (float2(4.0) * float2((-1.0), (-0.0))), (float2(4.0) * float2((-0.9238790), (-0.382684))), (float2(4.0) * float2((-0.707106), (-0.70710704))), (float2(4.0) * float2((-0.38268300), (-0.92388))), (float2(4.0) * float2(0.0, (-1.0))), (float2(4.0) * float2(0.382684, (-0.9238790))), (float2(4.0) * float2(0.70710704, (-0.70710704))), (float2(4.0) * float2(0.92388, (-0.38268300))), (float2(6.0) * float2(1.0, 0.0)), (float2(6.0) * float2(0.9659260, 0.25881902)), (float2(6.0) * float2(0.8660250, 0.5)), (float2(6.0) * float2(0.70710704, 0.70710704)), (float2(6.0) * float2(0.5, 0.8660260)), (float2(6.0) * float2(0.25881902, 0.9659260)), (float2(6.0) * float2((-0.0), 1.0)), (float2(6.0) * float2((-0.25881902), 0.9659260)), (float2(6.0) * float2((-0.5), 0.8660250)), (float2(6.0) * float2((-0.70710704), 0.70710704)), (float2(6.0) * float2((-0.8660260), 0.5)), (float2(6.0) * float2((-0.9659260), 0.25881902)), (float2(6.0) * float2((-1.0), (-0.0))), (float2(6.0) * float2((-0.9659260), (-0.25882000))), (float2(6.0) * float2((-0.8660250), (-0.5))), (float2(6.0) * float2((-0.707106), (-0.70710704))), (float2(6.0) * float2((-0.49999900), (-0.8660260))), (float2(6.0) * float2((-0.25881902), (-0.9659260))), (float2(6.0) * float2(0.0, (-1.0))), (float2(6.0) * float2(0.25881902, (-0.9659260))), (float2(6.0) * float2(0.5, (-0.8660250))), (float2(6.0) * float2(0.70710704, (-0.70710704))), (float2(6.0) * float2(0.8660260, (-0.49999900))), (float2(6.0) * float2(0.9659260, (-0.258818)))} };
    sampler samplerLinear;
    sampler samplerPoint;
    texture2d<float> TextureCoC;
    texture2d<float> TextureColor;
    texture2d<float> TextureColorMulFar;
    texture2d<float> TextureNearCoCBlurred;
    float4 Near(float2 texCoord, float2 pixelSize)
    {
        float4 result = TextureColor.sample(samplerPoint, texCoord);
        for (int i = 0; (i < 48); (i++))
        {
            float2 offset = ((float2(UniformDOF.maxRadius) * offsets[i]) * pixelSize);
            (result += TextureColor.sample(samplerLinear, (texCoord + offset)));
        }
        return (result / float4(49.0));
    };
    float4 Far(float2 texCoord, float2 pixelSize)
    {
        float4 result = TextureColorMulFar.sample(samplerPoint, texCoord);
        float weightsSum = (TextureCoC.sample(samplerPoint, texCoord)).y;
        for (int i = 0; (i < 48); (i++))
        {
            float2 offset = ((float2(UniformDOF.maxRadius) * offsets[i]) * pixelSize);
            float cocSample = (TextureCoC.sample(samplerLinear, (texCoord + offset))).y;
            float4 sample = TextureColorMulFar.sample(samplerLinear, (texCoord + offset));
            (result += sample);
            (weightsSum += cocSample);
        }
        return (result / float4(weightsSum));
    };
    PSOut main(VSOutput input)
    {
        uint w, h;
        w = TextureColor.get_width();
        h = TextureColor.get_height();
        float2 pixelSize = (float2(1.0) / float2((float)w, (float)h));
        PSOut output;
        float cocNearBlurred = (TextureNearCoCBlurred.sample(samplerPoint, (input).UV)).x;
        float cocFar = (TextureCoC.sample(samplerPoint, (input).UV)).y;
        float4 color = TextureColor.sample(samplerPoint, (input).UV);
        if (cocNearBlurred > 0.0)
        {
            ((output).TextureNear = Near((input).UV, pixelSize));
        }
        else
        {
            ((output).TextureNear = color);
        }
        if (cocFar > 0.0)
        {
            ((output).TextureFar = Far((input).UV, pixelSize));
        }
        else
        {
            ((output).TextureFar = float4(0.0));
        }
        return output;
    };

    Fragment_Shader(constant Uniforms_UniformDOF& UniformDOF, sampler samplerLinear, sampler samplerPoint, texture2d<float> TextureCoC, texture2d<float> TextureColor, texture2d<float> TextureColorMulFar, texture2d<float> TextureNearCoCBlurred) :
        UniformDOF(UniformDOF), samplerLinear(samplerLinear), samplerPoint(samplerPoint), TextureCoC(TextureCoC), TextureColor(TextureColor), TextureColorMulFar(TextureColorMulFar), TextureNearCoCBlurred(TextureNearCoCBlurred)
    {}
};

struct main_input
{
    float2 TEXCOORD0  [[user(locn0)]];
};

struct main_output
{
    float4 SV_Target0 [[color(0)]];
    float4 SV_Target1 [[color(1)]];
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
    texture2d<float> TextureColorMulFar [[id(2)]];
    texture2d<float> TextureNearCoCBlurred [[id(3)]];
    constant Fragment_Shader::Uniforms_UniformDOF& UniformDOF [[id(4)]];
};

fragment main_output stageMain(
	main_input inputData [[stage_in]],
    constant ArgBuffer0& argBuffer0 [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgBuffer1& argBuffer1 [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    Fragment_Shader::VSOutput input0;
    input0.UV = inputData.TEXCOORD0;
    Fragment_Shader main(argBuffer1.UniformDOF, argBuffer0.samplerLinear, argBuffer0.samplerPoint, argBuffer1.TextureCoC, argBuffer1.TextureColor, argBuffer1.TextureColorMulFar, argBuffer1.TextureNearCoCBlurred);
    Fragment_Shader::PSOut result = main.main(input0);
    main_output output;
    output.SV_Target0 = result.TextureNear;
    output.SV_Target1 = result.TextureFar;
    return output;
}
