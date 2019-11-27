struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

SamplerState	samplerLinear	: register(s0);
Texture2D		NearCoCTexture	: register(t0, UPDATE_FREQ_PER_FRAME);

struct PSOut
{
    float FilteredNearCoC : SV_Target0;
};

#define RADIUS 6

PSOut main(VSOutput input) : SV_TARGET
{
	PSOut output;
	
	uint w, h;
	NearCoCTexture.GetDimensions(w, h);
	float2 step = 1.0f / float2(w, h);
	
	for(int i = 0; i <= RADIUS * 2; ++i)
	{
		int index = (i - RADIUS);

		float2 coords;

#ifdef HORIZONTAL
			coords = input.UV + step * float2(float(index), 0.0);
#else
			coords = input.UV + step * float2(0.0, float(index));
#endif

        float sample = NearCoCTexture.Sample(samplerLinear, coords).r;  
		output.FilteredNearCoC += sample;
	}

	output.FilteredNearCoC = output.FilteredNearCoC / (RADIUS * 2.0f + 1.0f);

    return output;
}