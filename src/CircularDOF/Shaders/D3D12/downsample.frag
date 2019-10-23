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
    float2 DownresCoC	: SV_Target0;
    float4 DownresColor	: SV_Target1;
};

PSOut main(VSOutput input) : SV_TARGET
{
	PSOut output;

	output.DownresCoC = TextureCoC.Sample(samplerLinear, input.UV).rg;
	
	output.DownresColor = TextureColor.Sample(samplerLinear, input.UV);

    return output;
}