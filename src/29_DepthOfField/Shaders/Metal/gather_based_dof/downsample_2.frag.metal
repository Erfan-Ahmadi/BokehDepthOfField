#include <metal_stdlib>
using namespace metal;

struct Fragment_Shader
{
    struct VSOutput
    {
        float2 UV;
    };
    sampler samplerPoint;
    texture2d<float> TextureCoC;
    texture2d<float> TextureColor;
    struct PSOut
    {
        float4 DownresCoC;
        float4 DownresColor;
        float4 DownresFarMulColor;
    };
    PSOut main(VSOutput input)
    {
        PSOut output;
        float2 coc = (TextureCoC.sample(samplerPoint, (input).UV)).rg;
        float4 color = TextureColor.sample(samplerPoint, (input).UV);
        uint w, h;
        w = TextureColor.get_width();
        h = TextureColor.get_height();
        float2 pixelSize = (float2(1.0) / float2((float)w, (float)h));
        float2 texCoord00 = ((input).UV + (float2((-0.5), (-0.5)) * pixelSize));
        float2 texCoord10 = ((input).UV + (float2(0.5, (-0.5)) * pixelSize));
        float2 texCoord01 = ((input).UV + (float2((-0.5), 0.5) * pixelSize));
        float2 texCoord11 = ((input).UV + (float2(0.5, 0.5) * pixelSize));
        float cocFar00 = (TextureCoC.sample(samplerPoint, texCoord00)).g;
        float cocFar10 = (TextureCoC.sample(samplerPoint, texCoord10)).g;
        float cocFar01 = (TextureCoC.sample(samplerPoint, texCoord01)).g;
        float cocFar11 = (TextureCoC.sample(samplerPoint, texCoord11)).g;
        float weight00 = 1000.0;
        float4 colorMulCOCFar = (float4(weight00) * TextureColor.sample(samplerPoint, texCoord00));
        float weightsSum = weight00;
        float weight10 = (1.0 / (abs((cocFar00 - cocFar10)) + 0.0010000000));
        (colorMulCOCFar += (float4(weight10) * TextureColor.sample(samplerPoint, texCoord10)));
        (weightsSum += weight10);
        float weight01 = (1.0 / (abs((cocFar00 - cocFar01)) + 0.0010000000));
        (colorMulCOCFar += (float4(weight01) * TextureColor.sample(samplerPoint, texCoord01)));
        (weightsSum += weight01);
        float weight11 = (1.0 / (abs((cocFar00 - cocFar11)) + 0.0010000000));
        (colorMulCOCFar += (float4(weight11) * TextureColor.sample(samplerPoint, texCoord11)));
        (weightsSum += weight11);
        (colorMulCOCFar /= float4(weightsSum));
        (colorMulCOCFar *= float4(cocFar00));
        ((output).DownresCoC = float4((coc).r, cocFar00, 0, 0));
        ((output).DownresColor = color);
        ((output).DownresFarMulColor = colorMulCOCFar);
        return output;
    };

    Fragment_Shader(sampler samplerPoint, texture2d<float> TextureCoC, texture2d<float> TextureColor) :
        samplerPoint(samplerPoint), TextureCoC(TextureCoC), TextureColor(TextureColor)
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
    float4 SV_Target2 [[color(2)]];
};
struct ArgBuffer0
{
    sampler samplerPoint [[id(0)]];
};

struct ArgBuffer1
{
    texture2d<float> TextureCoC [[id(0)]];
    texture2d<float> TextureColor [[id(1)]];
};

fragment main_output stageMain(
	main_input inputData [[stage_in]],
    constant ArgBuffer0& argBuffer0 [[buffer(UPDATE_FREQ_NONE)]],
    constant ArgBuffer1& argBuffer1 [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    Fragment_Shader::VSOutput input0;
    input0.UV = inputData.TEXCOORD0;
    Fragment_Shader main(argBuffer0.samplerPoint, argBuffer1.TextureCoC, argBuffer1.TextureColor);
    Fragment_Shader::PSOut result = main.main(input0);
    main_output output;
    output.SV_Target0 = result.DownresCoC;
    output.SV_Target1 = result.DownresColor;
    output.SV_Target2 = result.DownresFarMulColor;
    return output;
}
