struct VSOutput 
{
	float4 Position : SV_POSITION;
	float4 Normal	: NORMAL;
	float3 Color	: COLOR;
};

SamplerState	uSampler0		: register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    return float4(input.Color, 1.0f);
}