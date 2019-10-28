struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

SamplerState	samplerLinear	: register(s0);
SamplerState	samplerPoint	: register(s1);

Texture2D		DepthTexture	: register(t0, UPDATE_FREQ_PER_FRAME);

cbuffer UniformDOF : register(b0, UPDATE_FREQ_PER_FRAME)
{
	float	maxRadius;
	float	blend;
	float	nearBegin;
	float	nearEnd;
	float	farBegin;
	float	farEnd;	
	float2 projParams;
};


float LinearizeDepth(float depth)
{
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

float2 main(VSOutput input): SV_Target
{
	float depth_ndc = DepthTexture.Sample(samplerPoint, input.UV).x;
	float depth = LinearizeDepth(depth_ndc);
	
	float nearCOC = 0.0f;
	if (depth < nearEnd)
		nearCOC = 1.0f/(nearBegin - nearEnd)*depth + -nearEnd/(nearBegin - nearEnd);
	else if (depth < nearBegin)
		nearCOC = 1.0f;
	nearCOC = saturate(nearCOC);
	
	float farCOC = 1.0f;
	if (depth < farBegin)
		farCOC = 0.0f;
	else if (depth < farEnd)
		farCOC = 1.0f/(farEnd - farBegin)*depth + -farBegin/(farEnd - farBegin);
	farCOC = saturate(farCOC);

	return float2(nearCOC, farCOC);
}
