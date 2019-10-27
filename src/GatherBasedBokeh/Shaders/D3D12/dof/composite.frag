
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

Texture2D		TextureColor		: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureCoC			: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureCoCDownres	: register(t2, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearCoCBlur	: register(t3, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureFar			: register(t4, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNear			: register(t5, UPDATE_FREQ_PER_FRAME);

float2 multComplex(float2 p, float2 q)
{
    return float2(p.x*q.x-p.y*q.y, p.x*q.y+p.y*q.x);
}

float4 main(VSOutput input) : SV_TARGET
{	
	uint w, h;
	TextureFarR.GetDimensions(w, h);
	float2 step = 1.0f / float2(w, h);

	float4 color = TextureColor.Sample(samplerLinear, input.UV);
	
	return color;
}