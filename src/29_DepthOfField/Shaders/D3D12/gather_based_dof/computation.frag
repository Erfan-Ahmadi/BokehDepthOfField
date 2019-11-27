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

struct PSOut
{
    float4 TextureNear			: SV_Target0;
    float4 TextureFar			: SV_Target1;
};

static const float2 offsets[] =
{
	2.0f * float2(1.000000f, 0.000000f),
	2.0f * float2(0.707107f, 0.707107f),
	2.0f * float2(-0.000000f, 1.000000f),
	2.0f * float2(-0.707107f, 0.707107f),
	2.0f * float2(-1.000000f, -0.000000f),
	2.0f * float2(-0.707106f, -0.707107f),
	2.0f * float2(0.000000f, -1.000000f),
	2.0f * float2(0.707107f, -0.707107f),
	
	4.0f * float2(1.000000f, 0.000000f),
	4.0f * float2(0.923880f, 0.382683f),
	4.0f * float2(0.707107f, 0.707107f),
	4.0f * float2(0.382683f, 0.923880f),
	4.0f * float2(-0.000000f, 1.000000f),
	4.0f * float2(-0.382684f, 0.923879f),
	4.0f * float2(-0.707107f, 0.707107f),
	4.0f * float2(-0.923880f, 0.382683f),
	4.0f * float2(-1.000000f, -0.000000f),
	4.0f * float2(-0.923879f, -0.382684f),
	4.0f * float2(-0.707106f, -0.707107f),
	4.0f * float2(-0.382683f, -0.923880f),
	4.0f * float2(0.000000f, -1.000000f),
	4.0f * float2(0.382684f, -0.923879f),
	4.0f * float2(0.707107f, -0.707107f),
	4.0f * float2(0.923880f, -0.382683f),

	6.0f * float2(1.000000f, 0.000000f),
	6.0f * float2(0.965926f, 0.258819f),
	6.0f * float2(0.866025f, 0.500000f),
	6.0f * float2(0.707107f, 0.707107f),
	6.0f * float2(0.500000f, 0.866026f),
	6.0f * float2(0.258819f, 0.965926f),
	6.0f * float2(-0.000000f, 1.000000f),
	6.0f * float2(-0.258819f, 0.965926f),
	6.0f * float2(-0.500000f, 0.866025f),
	6.0f * float2(-0.707107f, 0.707107f),
	6.0f * float2(-0.866026f, 0.500000f),
	6.0f * float2(-0.965926f, 0.258819f),
	6.0f * float2(-1.000000f, -0.000000f),
	6.0f * float2(-0.965926f, -0.258820f),
	6.0f * float2(-0.866025f, -0.500000f),
	6.0f * float2(-0.707106f, -0.707107f),
	6.0f * float2(-0.499999f, -0.866026f),
	6.0f * float2(-0.258819f, -0.965926f),
	6.0f * float2(0.000000f, -1.000000f),
	6.0f * float2(0.258819f, -0.965926f),
	6.0f * float2(0.500000f, -0.866025f),
	6.0f * float2(0.707107f, -0.707107f),
	6.0f * float2(0.866026f, -0.499999f),
	6.0f * float2(0.965926f, -0.258818f),
};

SamplerState	samplerLinear		: register(s0);
SamplerState	samplerPoint		: register(s1);

Texture2D		TextureCoC				: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColor			: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColorMulFar		: register(t2, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearCoCBlurred	: register(t3, UPDATE_FREQ_PER_FRAME);

float4 Near(float2 texCoord, float2 pixelSize)
{
	float4 result = TextureColor.Sample(samplerPoint, texCoord);
	
	for (int i = 0; i < 48; i++)
	{
		float2 offset = maxRadius * offsets[i] * pixelSize;
		result += TextureColor.Sample(samplerLinear, texCoord + offset);
	}

	return result / 49.0f;
}

float4 Far(float2 texCoord, float2 pixelSize)
{
	float4 result = TextureColorMulFar.Sample(samplerPoint, texCoord);
	float weightsSum = TextureCoC.Sample(samplerPoint, texCoord).y;
	
	for (int i = 0; i < 48; i++)
	{
		float2 offset = maxRadius * offsets[i] * pixelSize;
		
		float cocSample = TextureCoC.Sample(samplerLinear, texCoord + offset).y;
		float4 sample = TextureColorMulFar.Sample(samplerLinear, texCoord + offset);
		
		result += sample; // the texture is pre-multiplied so don't need to multiply here by weight
		weightsSum += cocSample;
	}

	return result / weightsSum;	
}

PSOut main(VSOutput input) : SV_TARGET
{
	uint w, h;
	TextureColor.GetDimensions(w, h);
	float2 pixelSize = 1.0f / float2(w, h);

	PSOut output;

	float cocNearBlurred = TextureNearCoCBlurred.Sample(samplerPoint, input.UV).x;
	float cocFar = TextureCoC.Sample(samplerPoint, input.UV).y;
	float4 color = TextureColor.Sample(samplerPoint, input.UV);

	if (cocNearBlurred > 0.0f)
		output.TextureNear = Near(input.UV, pixelSize);
	else
		output.TextureNear = color;

	if (cocFar > 0.0f)
		output.TextureFar = Far(input.UV, pixelSize);
	else
		output.TextureFar = 0.0f;

	return output;
}