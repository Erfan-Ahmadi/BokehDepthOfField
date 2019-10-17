struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};


SamplerState	uSampler0		: register(s0);
Texture2D		Texture			: register(t0, UPDATE_FREQ_PER_FRAME);

float4 main(VSOutput input) : SV_TARGET
{
    return Texture.Sample(uSampler0, input.UV);
}