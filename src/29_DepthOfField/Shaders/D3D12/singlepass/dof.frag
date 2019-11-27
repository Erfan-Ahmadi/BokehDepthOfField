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

SamplerState	samplerLinear	: register(s0);
SamplerState	samplerPoint	: register(s1);
Texture2D		TextureCoC		: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColor	: register(t1, UPDATE_FREQ_PER_FRAME);

static const float GOLDEN_ANGLE = 2.39996323f; 
static const float MAX_BLUR_SIZE = 20.0f; 
static const float RAD_SCALE = 1.5; // Smaller = nicer blur, larger = faster

float getBlurSize(float2 coc)
{
	if(coc.r > 0.0f)
		return coc.r * maxRadius * MAX_BLUR_SIZE * 5;
	return coc.g * maxRadius * MAX_BLUR_SIZE * 5;
}

float4 main(VSOutput input) : SV_TARGET
{
	uint w, h;
	TextureColor.GetDimensions(w, h);
	float2 pixelSize = 1.0f / float2(w, h);

	float3 color = TextureColor.Sample(samplerLinear, input.UV).rgb;
	
	float2 coc = TextureCoC.Sample(samplerPoint, input.UV).rg;

	float centerSize = getBlurSize(coc);

	float total = 1.0;
	float radius = RAD_SCALE;
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
	{
		float2 tc = input.UV + float2(cos(ang), sin(ang)) * pixelSize * radius;
		float3 sampleColor = TextureColor.Sample(samplerLinear, tc).rgb;

		float3 sampleCoC = TextureCoC.Sample(samplerPoint, tc).rgb;
		float sampleSize = getBlurSize(sampleCoC);

		//if (sampleSize > centerSize)
		//	sampleSize = clamp(sampleSize, 0.0, centerSize*2.0);

		float m = smoothstep(radius - 0.5f, radius + 0.5f, sampleSize);
		color += lerp(color/total, sampleColor, m);
		
		total += 1.0;   
		radius += RAD_SCALE / radius;
	}

	return float4(color / total, 1.0f);
}