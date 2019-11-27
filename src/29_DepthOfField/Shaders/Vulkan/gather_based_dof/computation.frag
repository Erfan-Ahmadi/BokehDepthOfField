#version 450 core

layout(UPDATE_FREQ_PER_FRAME, binding = 9) uniform UniformDOF
{
    float maxRadius;
    float blend;
};

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4	TextureNear; 
layout(location = 1) out vec4	TextureFar; 

const vec2 offsets[] = vec2[](
	2.0f * vec2(1.000000f, 0.000000f),
	2.0f * vec2(0.707107f, 0.707107f),
	2.0f * vec2(-0.000000f, 1.000000f),
	2.0f * vec2(-0.707107f, 0.707107f),
	2.0f * vec2(-1.000000f, -0.000000f),
	2.0f * vec2(-0.707106f, -0.707107f),
	2.0f * vec2(0.000000f, -1.000000f),
	2.0f * vec2(0.707107f, -0.707107f),
	
	4.0f * vec2(1.000000f, 0.000000f),
	4.0f * vec2(0.923880f, 0.382683f),
	4.0f * vec2(0.707107f, 0.707107f),
	4.0f * vec2(0.382683f, 0.923880f),
	4.0f * vec2(-0.000000f, 1.000000f),
	4.0f * vec2(-0.382684f, 0.923879f),
	4.0f * vec2(-0.707107f, 0.707107f),
	4.0f * vec2(-0.923880f, 0.382683f),
	4.0f * vec2(-1.000000f, -0.000000f),
	4.0f * vec2(-0.923879f, -0.382684f),
	4.0f * vec2(-0.707106f, -0.707107f),
	4.0f * vec2(-0.382683f, -0.923880f),
	4.0f * vec2(0.000000f, -1.000000f),
	4.0f * vec2(0.382684f, -0.923879f),
	4.0f * vec2(0.707107f, -0.707107f),
	4.0f * vec2(0.923880f, -0.382683f),

	6.0f * vec2(1.000000f, 0.000000f),
	6.0f * vec2(0.965926f, 0.258819f),
	6.0f * vec2(0.866025f, 0.500000f),
	6.0f * vec2(0.707107f, 0.707107f),
	6.0f * vec2(0.500000f, 0.866026f),
	6.0f * vec2(0.258819f, 0.965926f),
	6.0f * vec2(-0.000000f, 1.000000f),
	6.0f * vec2(-0.258819f, 0.965926f),
	6.0f * vec2(-0.500000f, 0.866025f),
	6.0f * vec2(-0.707107f, 0.707107f),
	6.0f * vec2(-0.866026f, 0.500000f),
	6.0f * vec2(-0.965926f, 0.258819f),
	6.0f * vec2(-1.000000f, -0.000000f),
	6.0f * vec2(-0.965926f, -0.258820f),
	6.0f * vec2(-0.866025f, -0.500000f),
	6.0f * vec2(-0.707106f, -0.707107f),
	6.0f * vec2(-0.499999f, -0.866026f),
	6.0f * vec2(-0.258819f, -0.965926f),
	6.0f * vec2(0.000000f, -1.000000f),
	6.0f * vec2(0.258819f, -0.965926f),
	6.0f * vec2(0.500000f, -0.866025f),
	6.0f * vec2(0.707107f, -0.707107f),
	6.0f * vec2(0.866026f, -0.499999f),
	6.0f * vec2(0.965926f, -0.258818f)
);

layout(binding = 1) uniform sampler samplerLinear;
layout(binding = 2) uniform sampler samplerPoint;

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform texture2D TextureCoC;
layout(UPDATE_FREQ_PER_FRAME, binding = 1) uniform texture2D TextureColor;
layout(UPDATE_FREQ_PER_FRAME, binding = 2) uniform texture2D TextureColorMulFar;
layout(UPDATE_FREQ_PER_FRAME, binding = 3) uniform texture2D TextureNearCoCBlurred;

vec4 Near(vec2 texCoord, vec2 pixelSize)
{
	vec4 result =  texture(sampler2D(TextureColor, samplerPoint), texCoord);
	
	for (int i = 0; i < 48; i++)
	{
		vec2 offset = maxRadius * offsets[i] * pixelSize;
		result +=  texture(sampler2D(TextureColor, samplerLinear), texCoord + offset);
	}

	return result / 49.0f;
}

vec4 Far(vec2 texCoord, vec2 pixelSize)
{
	vec4 result =  texture(sampler2D(TextureColorMulFar, samplerPoint), texCoord);
	float weightsSum =  texture(sampler2D(TextureCoC, samplerPoint), texCoord).y;
	
	for (int i = 0; i < 48; i++)
	{
		vec2 offset = maxRadius * offsets[i] * pixelSize;
		
		float cocSample =  texture(sampler2D(TextureCoC, samplerLinear), texCoord + offset).y;

		vec4 texel =  texture(sampler2D(TextureColorMulFar, samplerLinear), texCoord + offset);
		
		weightsSum += cocSample;
		result += texel; // the texture is pre-multiplied so don't need to multiply here by weight
	}

	return result / weightsSum;	
}

void main()
{
	vec2 pixelSize = 1.0f / textureSize(sampler2D(TextureColor, samplerPoint), 0);

	float cocNearBlurred =  texture(sampler2D(TextureNearCoCBlurred, samplerPoint), inUV).x;
	float cocFar =  texture(sampler2D(TextureCoC, samplerPoint), inUV).y;
	vec4 color =  texture(sampler2D(TextureColor, samplerPoint), inUV);

	if (cocNearBlurred > 0.0f)
		TextureNear = Near(inUV, pixelSize);
	else
		TextureNear = color;

	if (cocFar > 0.0f)
		TextureFar = Far(inUV, pixelSize);
	else
		TextureFar = vec4(0.0f);
}