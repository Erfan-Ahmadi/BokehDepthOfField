#version 450 core

layout(location = 0) in vec2 UV;

layout (binding = 1) uniform sampler								samplerLinear;
layout (UPDATE_FREQ_PER_FRAME, binding = 2) uniform texture2D		NearCoCTexture;

layout (std140, UPDATE_FREQ_PER_FRAME, binding = 3) uniform BlurData
{
	uniform float BlurStrength;
};

layout(location = 0) out float outNearCoC;

#define RADIUS 6

void main()
{
	vec2 step = vec2(1.0f / textureSize(sampler2D(NearCoCTexture, samplerLinear),0).r);
	
	outNearCoC = 0;
		
	for(int i = 0; i <= RADIUS * 2; ++i)
	{
		int index = (i - RADIUS);

		vec2 coords;

#ifdef HORIZONTAL
			coords = UV + step * vec2(float(index), 0.0);
#else							 
			coords = UV + step * vec2(0.0, float(index));
#endif

        outNearCoC += texture(sampler2D(NearCoCTexture, samplerLinear), coords).r;
	}

	outNearCoC = outNearCoC / (RADIUS * 2.0f + 1.0f);
}