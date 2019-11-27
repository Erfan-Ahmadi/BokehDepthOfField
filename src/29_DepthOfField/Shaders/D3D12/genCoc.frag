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
	float	nb;
	float	ne;
	float	fb;
	float	fe;	
	float2 projParams;
};


float LinearizeDepth(float depth)
{
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

float2 main(VSOutput input): SV_Target
{
	float depth = DepthTexture.Sample(samplerPoint, input.UV).x;

	float depth_linearized = LinearizeDepth(depth);

	float r = (ne - depth_linearized) / (ne - nb);
	float g = (depth_linearized - fb) / (fe - fb);

	return float2(r, g);
}
