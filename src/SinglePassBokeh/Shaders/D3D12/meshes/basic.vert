struct VsIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float2 texCoord : TEXCOORD;
};

cbuffer cbPerPass : register(b0, UPDATE_FREQ_PER_FRAME) 
{
	float4x4	proj;
	float4x4	view;
}

cbuffer cbPerProp : register(b1)
{
	float4x4	world;
}

struct PsIn
{    
    float3 normal	: TEXCOORD0;
	float3 pos		: TEXCOORD1;
	float2 uv		: TEXCOORD2;
    float4 position : SV_Position;
};

PsIn main(VsIn In)
{
	PsIn Out;

	float4x4 projView = mul(proj, view);

	Out.position = mul(projView, mul(world, float4(In.position.xyz, 1.0f)));
	Out.normal = mul((float3x3)world, In.normal.xyz).xyz;

	Out.pos = mul(world, float4(In.position.xyz, 1.0f)).xyz;
	Out.uv = In.texCoord.xy;

	return Out;
}