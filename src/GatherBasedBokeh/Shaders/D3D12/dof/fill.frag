struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

struct PSOut
{
    float4 TextureNearFilled	: SV_Target0;
    float4 TextureFarFilled		: SV_Target1;
};

SamplerState	samplerLinear		: register(s0);
SamplerState	samplerPoint		: register(s1);

Texture2D		TextureCoC			: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearCoCBlur	: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureFar			: register(t2, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNear			: register(t3, UPDATE_FREQ_PER_FRAME);

PSOut main(VSOutput input) : SV_TARGET
{
	uint w, h;
	TextureFar.GetDimensions(w, h);
	float2 pixelSize = 1.0f / float2(w, h);

	PSOut output;
	
	output.TextureNearFilled = TextureNear.Sample(samplerPoint, input.UV);
	output.TextureFarFilled = TextureFar.Sample(samplerPoint, input.UV);	
	
	float cocNearBlurred = TextureNearCoCBlur.Sample(samplerPoint, input.UV).x;
	float cocFar = TextureCoC.Sample(samplerPoint, input.UV).y;

	if (cocNearBlurred > 0.0f)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				float2 sampleTexCoord = input.UV + float2(i, j) * pixelSize;
				float4 sample = TextureNear.Sample(samplerPoint, sampleTexCoord);
				output.TextureNearFilled = max(output.TextureNearFilled, sample);
			}
		}
	}

	if (cocFar > 0.0f)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				float2 sampleTexCoord = input.UV + float2(i, j) * pixelSize;
				float4 sample = TextureFar.Sample(samplerPoint, sampleTexCoord);
				output.TextureFarFilled = max(output.TextureFarFilled, sample);
			}
		}
	}

	return output;
}