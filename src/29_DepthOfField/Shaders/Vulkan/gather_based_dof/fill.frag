#version 450 core

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4	TextureNearFilled; 
layout(location = 1) out vec4	TextureFarFilled; 

layout(binding = 1) uniform sampler samplerLinear;
layout(binding = 2) uniform sampler samplerPoint;

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform texture2D TextureCoC;
layout(UPDATE_FREQ_PER_FRAME, binding = 1) uniform texture2D TextureNearCoCBlur;
layout(UPDATE_FREQ_PER_FRAME, binding = 2) uniform texture2D TextureFar;
layout(UPDATE_FREQ_PER_FRAME, binding = 3) uniform texture2D TextureNear;

void main()
{
	vec2 pixelSize = 1.0f / textureSize(sampler2D(TextureFar, samplerPoint), 0);

	
	TextureNearFilled = texture(sampler2D(TextureNear, samplerPoint), inUV);
	TextureFarFilled = texture(sampler2D(TextureFar, samplerPoint), inUV);	

	float cocNearBlurred = texture(sampler2D(TextureNearCoCBlur, samplerPoint), inUV).r;	
	float cocFar = texture(sampler2D(TextureCoC, samplerPoint), inUV).g;	

	if (cocNearBlurred > 0.0f)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				vec2 sampleTexCoord = inUV + vec2(i, j) * pixelSize;
				vec4 texel = texture(sampler2D(TextureNear, samplerPoint), sampleTexCoord);
				TextureNearFilled = max(TextureNearFilled, texel);
			}
		}
	}

	if (cocFar > 0.0f)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				vec2 sampleTexCoord = inUV + vec2(i, j) * pixelSize;
				vec4 texel = texture(sampler2D(TextureFar, samplerPoint), sampleTexCoord);
				TextureFarFilled = max(TextureFarFilled, texel);
			}
		}
	}
}