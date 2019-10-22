struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

SamplerState	samplerLinear	: register(s0);
Texture2D		DepthTexture	: register(t0, UPDATE_FREQ_PER_FRAME);

cbuffer cbPerPass : register(b0, UPDATE_FREQ_PER_FRAME) 
{
	float4x4	proj;
	float4x4	view;
}

cbuffer UniformDOF : register(b0, UPDATE_FREQ_PER_FRAME)
{
	float	filterRadius;
	float	nb;
	float	ne;
	float	fb;
	float	fe;
	float3	pad;
};

struct PSOut
{
    float2 COC : SV_Target0;
};

float LinearizeDepth(float depth)
{
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

float DepthNDCToView(float depth_ndc)
{
	return -proj[2][2] / (depth_ndc + proj[3][2]);
}

PSOut main(VSOutput input) : SV_TARGET
{
	PSOut output;

	float depth = DepthTexture.Sample(samplerLinear, input.UV).x;

	float depth_linearized = LinearizeDepth(depth);

	float r = (ne - depth_linearized) / (ne - nb);
	float g = (depth_linearized - fb) / (fe - fb);

	output.COC = float2(r, g);

    return output;
}