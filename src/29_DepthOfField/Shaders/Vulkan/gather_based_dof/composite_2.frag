#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor; 

layout(UPDATE_FREQ_PER_FRAME, binding = 9) uniform UniformDOF
{
    float maxRadius;
    float blend;
};

layout(set = 0, binding = 0) uniform sampler samplerLinear;
layout(set = 0, binding = 1) uniform sampler samplerPoint;

layout(set = 1, binding = 0) uniform texture2D TextureColor;		
layout(set = 1, binding = 1) uniform texture2D TextureCoC;			
layout(set = 1, binding = 2) uniform texture2D TextureCoC_x4;		
layout(set = 1, binding = 3) uniform texture2D TextureNearCoCBlur;	
layout(set = 1, binding = 4) uniform texture2D TextureFar_x4;		
layout(set = 1, binding = 5) uniform texture2D TextureNear_x4;

void main()
{
	vec2 pixelSize = 1.0f / textureSize(sampler2D(TextureColor, samplerPoint), 0);
	
	vec4 color =  texture(sampler2D(TextureColor, samplerPoint), inUV);

	// Far
	{
		vec2 texCoord00 = inUV;
		vec2 texCoord10 = inUV + vec2(pixelSize.x, 0.0f);
		vec2 texCoord01 = inUV + vec2(0.0f, pixelSize.y);
		vec2 texCoord11 = inUV + vec2(pixelSize.x, pixelSize.y);

		float cocFar =  texture(sampler2D(TextureCoC, samplerPoint), inUV).y;
		//vec4 cocsFar_x4 = TextureCoC_x4.GatherGreen(samplerPoint, texCoord00).wzxy;
		vec4 cocsFar_x4 = vec4(texture(sampler2D(TextureCoC_x4, samplerPoint), texCoord00).y);
		vec4 cocsFarDiffs = abs(cocFar.xxxx - cocsFar_x4);

		vec4 dofFar00 =  texture(sampler2D(TextureFar_x4, samplerPoint), texCoord00);
		vec4 dofFar10 =  texture(sampler2D(TextureFar_x4, samplerPoint), texCoord10);
		vec4 dofFar01 =  texture(sampler2D(TextureFar_x4, samplerPoint), texCoord01);
		vec4 dofFar11 =  texture(sampler2D(TextureFar_x4, samplerPoint), texCoord11);

		vec2 imageCoord = inUV / pixelSize;
		vec2 fractional = fract(imageCoord);
		float a = (1.0f - fractional.x) * (1.0f - fractional.y);
		float b = fractional.x * (1.0f - fractional.y);
		float c = (1.0f - fractional.x) * fractional.y;
		float d = fractional.x * fractional.y;

		vec4 dofFar = vec4(0.0f);
		float weightsSum = 0.0f;

		float weight00 = a / (cocsFarDiffs.x + 0.001f);
		dofFar += weight00 * dofFar00;
		weightsSum += weight00;

		float weight10 = b / (cocsFarDiffs.y + 0.001f);
		dofFar += weight10 * dofFar10;
		weightsSum += weight10;

		float weight01 = c / (cocsFarDiffs.z + 0.001f);
		dofFar += weight01 * dofFar01;
		weightsSum += weight01;

		float weight11 = d / (cocsFarDiffs.w + 0.001f);
		dofFar += weight11 * dofFar11;
		weightsSum += weight11;

		dofFar /= weightsSum;

		color = mix(color, dofFar, clamp(blend * cocFar, 0.0f, 1.0f));
	}

	// Near
	{
		float cocNear =  texture(sampler2D(TextureNearCoCBlur, samplerLinear), inUV).x;
		vec4 dofNear =  texture(sampler2D(TextureNear_x4, samplerLinear), inUV);

		color = mix(color, dofNear, clamp(blend * cocNear, 0.0f, 1.0f));
	}
	
	outColor = color;
}