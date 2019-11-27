#include <metal_stdlib>
using namespace metal;

struct Fragment_Shader
{
    struct VSOutput
    {
        float2 UV;
    };
    struct PSOut
    {
        float4 TextureNearFilled;
        float4 TextureFarFilled;
    };
    sampler samplerPoint;
    texture2d<float> TextureCoC;
    texture2d<float> TextureNearCoCBlur;
    texture2d<float> TextureFar;
    texture2d<float> TextureNear;
    PSOut main(VSOutput input)
    {
        uint w, h;
        w = TextureFar.get_width();
        h = TextureFar.get_height();
        float2 pixelSize = (float2(1.0) / float2((float)w, (float)h));
        PSOut output;
        ((output).TextureNearFilled = TextureNear.sample(samplerPoint, (input).UV));
        ((output).TextureFarFilled = TextureFar.sample(samplerPoint, (input).UV));
        float cocNearBlurred = (TextureNearCoCBlur.sample(samplerPoint, (input).UV)).x;
        float cocFar = (TextureCoC.sample(samplerPoint, (input).UV)).y;
        if (cocNearBlurred > 0.0)
        {
            for (int i = (-1); (i <= 1); (i++))
            {
                for (int j = (-1); (j <= 1); (j++))
                {
                    float2 sampleTexCoord = ((input).UV + (float2((float)i, (float)j) * pixelSize));
                    float4 sample = TextureNear.sample(samplerPoint, sampleTexCoord);
                    ((output).TextureNearFilled = max((float4)output.TextureNearFilled,(float4)sample));
                }
            }
        }
        if (cocFar > 0.0)
        {
            for (int i = (-1); (i <= 1); (i++))
            {
                for (int j = (-1); (j <= 1); (j++))
                {
                    float2 sampleTexCoord = ((input).UV + (float2((float)i, (float)j) * pixelSize));
                    float4 sample = TextureFar.sample(samplerPoint, sampleTexCoord);
                    ((output).TextureFarFilled = max((float4)output.TextureFarFilled,(float4)sample));
                }
            }
        }
        return output;
    };

    Fragment_Shader(sampler samplerPoint, texture2d<float> TextureCoC, texture2d<float> TextureNearCoCBlur, texture2d<float> TextureFar, texture2d<float> TextureNear) :
        samplerPoint(samplerPoint), TextureCoC(TextureCoC), TextureNearCoCBlur(TextureNearCoCBlur), TextureFar(TextureFar), TextureNear(TextureNear)
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
    sampler samplerPoint [[id(0)]];
};

struct ArgBuffer1
{
    texture2d<float> TextureCoC [[id(0)]];
    texture2d<float> TextureNearCoCBlur [[id(1)]];
    texture2d<float> TextureFar [[id(2)]];
    texture2d<float> TextureNear [[id(3)]];
};

fragment main_output stageMain(
	main_input inputData [[stage_in]],
    constant ArgBuffer0& argBuffer0 [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgBuffer1& argBuffer1 [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    Fragment_Shader::VSOutput input0;
    input0.UV = inputData.TEXCOORD0;
    Fragment_Shader main(argBuffer0.samplerPoint, argBuffer1.TextureCoC, argBuffer1.TextureNearCoCBlur, argBuffer1.TextureFar, argBuffer1.TextureNear);
    Fragment_Shader::PSOut result = main.main(input0);
    main_output output;
    output.SV_Target0 = result.TextureNearFilled;
    output.SV_Target1 = result.TextureFarFilled;
    return output;
}
