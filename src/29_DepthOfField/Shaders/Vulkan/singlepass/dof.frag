#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outcolor; 

layout(UPDATE_FREQ_PER_FRAME, binding = 9) uniform UniformDOF
{
    float maxRadius;
    float blend;
};

layout(binding = 1) uniform sampler samplerLinear;
layout(binding = 2) uniform sampler samplerPoint;

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform texture2D TextureCoC;
layout(UPDATE_FREQ_PER_FRAME, binding = 1) uniform texture2D TextureColor;

const float GOLDEN_ANGLE = 2.39996323f; 
const float MAX_BLUR_SIZE = 20.0f; 
const float RAD_SCALE = 1.5; // Smaller = nicer blur, larger = faster

float getBlurSize(vec2 coc)
{
	if(coc.r > 0.0f)
		return coc.r * maxRadius * MAX_BLUR_SIZE * 5;
	return coc.g * maxRadius * MAX_BLUR_SIZE * 5;
}

void main()
{
	vec2 pixelSize = 1.0f / textureSize(sampler2D(TextureColor, samplerPoint), 0);

	vec3 color = texture(sampler2D(TextureColor, samplerLinear), inUV).rgb;
	
	vec2 coc = texture(sampler2D(TextureCoC, samplerPoint), inUV).rg;

	float centerSize = getBlurSize(coc);

	float total = 1.0;
	float radius = RAD_SCALE;
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
	{
		vec2 coords = inUV + vec2(cos(ang), sin(ang)) * pixelSize * radius;
		vec3 sampleColor = texture(sampler2D(TextureColor, samplerLinear), coords).rgb;

		vec2 sampleCoC = texture(sampler2D(TextureCoC, samplerPoint), coords).rg;
		float sampleSize = getBlurSize(sampleCoC);

		//if (sampleSize > centerSize)
		//	sampleSize = clamp(sampleSize, 0.0, centerSize*2.0);

		float m = smoothstep(radius - 0.5f, radius + 0.5f, sampleSize);
		color += mix(color/total, sampleColor, m);
		
		total += 1.0;   
		radius += RAD_SCALE / radius;
	}

	outcolor = vec4(color / total, 1.0f);
}