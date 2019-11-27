
cbuffer cbPerPass : register(b0, UPDATE_FREQ_PER_FRAME) 
{
	float4x4	proj;
	float4x4	view;
}

struct VSInput
{
    float3 Position			: POSITION;
    float3 InstancePosition : NORMAL;
    float3 InstanceColor	: COLOR;
};

struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float3 Color	: COLOR;
};

VSOutput main(VSInput input)
{
	VSOutput result;
	float4 worldPos = float4(input.InstancePosition + 0.5f * input.Position * float3(1, 1, 1), 1.0f); 
	result.Position = mul(proj, mul(view,  worldPos));
	result.Color = input.InstanceColor;
	return result;
}