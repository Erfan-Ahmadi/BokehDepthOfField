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

Texture2D		TextureCoC			: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColor		: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColorMulFar	: register(t2, UPDATE_FREQ_PER_FRAME);

struct PSOut
{
    float4 TextureFar			: SV_Target0;
    float2 TextureNear			: SV_Target1;
};

PSOut main(VSOutput input) : SV_TARGET
{
	PSOut output;

	output.TextureFar = float4(0, 0, 0, 0);
	output.TextureNear = float4(0, 0, 0, 0);

	return output;
}