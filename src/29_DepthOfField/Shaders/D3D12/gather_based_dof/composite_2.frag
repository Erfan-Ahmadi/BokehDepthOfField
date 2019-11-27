
cbuffer UniformDOF : register(b0, UPDATE_FREQ_PER_FRAME)
{
	float	maxRadius;
	float	blend;
};

struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

SamplerState	samplerLinear		: register(s0);
SamplerState	samplerPoint		: register(s1);

Texture2D		TextureColor			: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureCoC				: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureCoC_x4			: register(t2, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearCoCBlur		: register(t3, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureFar_x4			: register(t4, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNear_x4			: register(t5, UPDATE_FREQ_PER_FRAME);

float4 main(VSOutput input) : SV_TARGET
{	
	uint w, h;
	TextureColor.GetDimensions(w, h);
	float2 pixelSize = (1.0f / float2(w, h));
	
	float4 color = TextureColor.Sample(samplerPoint, input.UV);

	// Far
	{
		float2 texCoord00 = input.UV;
		float2 texCoord10 = input.UV + float2(pixelSize.x, 0.0f);
		float2 texCoord01 = input.UV + float2(0.0f, pixelSize.y);
		float2 texCoord11 = input.UV + float2(pixelSize.x, pixelSize.y);

		float cocFar = TextureCoC.Sample(samplerPoint, input.UV).y;
		float4 cocsFar_x4 = TextureCoC_x4.GatherGreen(samplerPoint, texCoord00).wzxy;
		float4 cocsFarDiffs = abs(cocFar.xxxx - cocsFar_x4);

		float4 dofFar00 = TextureFar_x4.Sample(samplerPoint, texCoord00);
		float4 dofFar10 = TextureFar_x4.Sample(samplerPoint, texCoord10);
		float4 dofFar01 = TextureFar_x4.Sample(samplerPoint, texCoord01);
		float4 dofFar11 = TextureFar_x4.Sample(samplerPoint, texCoord11);

		float2 imageCoord = input.UV / pixelSize;
		float2 fractional = frac(imageCoord);
		float a = (1.0f - fractional.x) * (1.0f - fractional.y);
		float b = fractional.x * (1.0f - fractional.y);
		float c = (1.0f - fractional.x) * fractional.y;
		float d = fractional.x * fractional.y;

		float4 dofFar = 0.0f;
		float weightsSum = 0.0f;

		float weight00 = a / (cocsFarDiffs.x + 0.001f);
		dofFar += weight00 * dofFar00;
		weightsSum += weight00;

		float weight10 = b / (cocsFarDiffs.y + 0.001f);
		dofFar += weight10 * dofFar10;
		weightsSum += weight10;

		float weight01 = c / (cocsFarDiffs.z + 0.001f);
		dofFar += weight01 * dofFar01;
		weightsSum += weight01;

		float weight11 = d / (cocsFarDiffs.w + 0.001f);
		dofFar += weight11 * dofFar11;
		weightsSum += weight11;

		dofFar /= weightsSum;

		color = lerp(color, dofFar, saturate(blend * cocFar));
	}

	// Near
	{
		float cocNear = TextureNearCoCBlur.Sample(samplerLinear, input.UV).x;
		float4 dofNear = TextureNear_x4.Sample(samplerLinear, input.UV);

		color = lerp(color, dofNear, saturate(blend * cocNear));
	}
	
	return color;
}