struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

VSOutput main(uint VertexID: SV_VertexID)
{
	VSOutput result;
	
	result.UV = float2((VertexID << 1) & 2, VertexID & 2);
	result.Position = float4(float2(result.UV * 2.0f - 1.0f) * float2(1, -1), 0.0f, 1.0f);

	return result;
}