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
    texture2d<float> TextureColor;
    texture2d<float> TextureCoC;
    texture2d<float> TextureCoC_x4;
    texture2d<float> TextureNearCoCBlur;
    texture2d<float> TextureFar_x4;
    texture2d<float> TextureNear_x4;
    float4 main(VSOutput input)
    {
        uint w, h;
        w = TextureColor.get_width();
        h = TextureColor.get_height();
        float2 pixelSize = (float2(1.0) / float2((float)w, (float)h));
        float4 color = TextureColor.sample(samplerPoint, (input).UV);
        {
            float2 texCoord00 = (input).UV;
            float2 texCoord10 = ((input).UV + float2((pixelSize).x, 0.0));
            float2 texCoord01 = ((input).UV + float2(0.0, (pixelSize).y));
            float2 texCoord11 = ((input).UV + float2((pixelSize).x, (pixelSize).y));
            float cocFar = (TextureCoC.sample(samplerPoint, (input).UV)).y;
            float4 cocsFar_x4 = float4((TextureCoC_x4.sample(samplerPoint, texCoord00)).g);
            float4 cocsFarDiffs = abs((float4(cocFar) - cocsFar_x4));
            float4 dofFar00 = TextureFar_x4.sample(samplerPoint, texCoord00);
            float4 dofFar10 = TextureFar_x4.sample(samplerPoint, texCoord10);
            float4 dofFar01 = TextureFar_x4.sample(samplerPoint, texCoord01);
            float4 dofFar11 = TextureFar_x4.sample(samplerPoint, texCoord11);
            float2 imageCoord = ((input).UV / pixelSize);
            float2 fractional = fract(imageCoord);
            float a = ((1.0 - (fractional).x) * (1.0 - (fractional).y));
            float b = ((fractional).x * (1.0 - (fractional).y));
            float c = ((1.0 - (fractional).x) * (fractional).y);
            float d = ((fractional).x * (fractional).y);
            float4 dofFar = float4(0.0);
            float weightsSum = 0.0;
            float weight00 = (a / ((cocsFarDiffs).x + 0.0010000000));
            (dofFar += (float4(weight00) * dofFar00));
            (weightsSum += weight00);
            float weight10 = (b / ((cocsFarDiffs).y + 0.0010000000));
            (dofFar += (float4(weight10) * dofFar10));
            (weightsSum += weight10);
            float weight01 = (c / ((cocsFarDiffs).z + 0.0010000000));
            (dofFar += (float4(weight01) * dofFar01));
            (weightsSum += weight01);
            float weight11 = (d / ((cocsFarDiffs).w + 0.0010000000));
            (dofFar += (float4(weight11) * dofFar11));
            (weightsSum += weight11);
            (dofFar /= float4(weightsSum));
            (color = mix(color, dofFar, float4(saturate((UniformDOF.blend * cocFar)))));
        }
        {
            float cocNear = (TextureNearCoCBlur.sample(samplerLinear, (input).UV)).x;
            float4 dofNear = TextureNear_x4.sample(samplerLinear, (input).UV);
            (color = mix(color, dofNear, float4(saturate((UniformDOF.blend * cocNear)))));
        }
        return color;
    };

    Fragment_Shader(constant Uniforms_UniformDOF& UniformDOF, sampler samplerLinear, sampler samplerPoint, texture2d<float> TextureColor, texture2d<float> TextureCoC, texture2d<float> TextureCoC_x4, texture2d<float> TextureNearCoCBlur, texture2d<float> TextureFar_x4, texture2d<float> TextureNear_x4) :
        UniformDOF(UniformDOF), samplerLinear(samplerLinear), samplerPoint(samplerPoint), TextureColor(TextureColor), TextureCoC(TextureCoC), TextureCoC_x4(TextureCoC_x4), TextureNearCoCBlur(TextureNearCoCBlur), TextureFar_x4(TextureFar_x4), TextureNear_x4(TextureNear_x4)
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
    texture2d<float> TextureColor [[id(0)]];
    texture2d<float> TextureCoC [[id(1)]];
    texture2d<float> TextureCoC_x4 [[id(2)]];
    texture2d<float> TextureNearCoCBlur [[id(3)]];
    texture2d<float> TextureFar_x4 [[id(4)]];
    texture2d<float> TextureNear_x4 [[id(5)]];
    constant Fragment_Shader::Uniforms_UniformDOF& UniformDOF [[id(6)]];
};

fragment float4 stageMain(
	main_input inputData [[stage_in]],
    constant ArgBuffer0& argBuffer0 [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgBuffer1& argBuffer1 [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    Fragment_Shader::VSOutput input0;
    input0.UV = inputData.TEXCOORD0;
    Fragment_Shader main(argBuffer1.UniformDOF, argBuffer0.samplerLinear, argBuffer0.samplerPoint, argBuffer1.TextureColor, argBuffer1.TextureCoC, argBuffer1.TextureCoC_x4, argBuffer1.TextureNearCoCBlur, argBuffer1.TextureFar_x4, argBuffer1.TextureNear_x4);
    return main.main(input0);
}
