struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

SamplerState	samplerLinear	: register(s0);
SamplerState	samplerPoint	: register(s1);
Texture2D		TextureCoC		: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColor	: register(t1, UPDATE_FREQ_PER_FRAME);

struct PSOut
{
    float2 DownresCoC			: SV_Target0;
    float4 DownresColor			: SV_Target1;
    float4 DownresFarMulColor	: SV_Target2;
};

PSOut main(VSOutput input) : SV_TARGET
{
	PSOut output;

	float2 coc = TextureCoC.Sample(samplerPoint, input.UV).rg;
	float4 color = TextureColor.Sample(samplerPoint, input.UV);
	
	uint w, h;
	TextureColor.GetDimensions(w, h);
	float2 pixelSize = 1.0f / float2(w, h);

	float2 texCoord00 = input.UV + float2(-0.5f, -0.5f) * pixelSize;
	float2 texCoord10 = input.UV + float2( 0.5f, -0.5f) * pixelSize;
	float2 texCoord01 = input.UV + float2(-0.5f,  0.5f) * pixelSize;
	float2 texCoord11 = input.UV + float2( 0.5f,  0.5f) * pixelSize;
	
	float cocFar00 = TextureCoC.Sample(samplerPoint, texCoord00).g;
	float cocFar10 = TextureCoC.Sample(samplerPoint, texCoord10).g;
	float cocFar01 = TextureCoC.Sample(samplerPoint, texCoord01).g;
	float cocFar11 = TextureCoC.Sample(samplerPoint, texCoord11).g;

	// Closer The Samples -> Bigger Contributions

	float weight00 = 1000.0f;
	float4 colorMulCOCFar = weight00 * TextureColor.Sample(samplerPoint, texCoord00);
	float weightsSum = weight00;
	
	float weight10 = 1.0f / (abs(cocFar00 - cocFar10) + 0.001f);
	colorMulCOCFar += weight10 * TextureColor.Sample(samplerPoint, texCoord10);
	weightsSum += weight10;
	
	float weight01 = 1.0f / (abs(cocFar00 - cocFar01) + 0.001f);
	colorMulCOCFar += weight01 * TextureColor.Sample(samplerPoint, texCoord01);
	weightsSum += weight01;
	
	float weight11 = 1.0f / (abs(cocFar00 - cocFar11) + 0.001f);
	colorMulCOCFar += weight11 * TextureColor.Sample(samplerPoint, texCoord11);
	weightsSum += weight11;

	colorMulCOCFar /= weightsSum;
	colorMulCOCFar *= cocFar00;
	
    output.DownresCoC = float2(coc.r, cocFar00);
    output.DownresColor = color;
    output.DownresFarMulColor = colorMulCOCFar;

    return output;
}