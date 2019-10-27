
struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float3 Color	: COLOR;
};

struct PSOut
{
    float4 color	: SV_Target0;
};

PSOut main(VSOutput input) : SV_TARGET
{
	PSOut output;
    output.color = float4(input.Color, 1.0f);
	return output;
}