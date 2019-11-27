#version 450 core

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec2 outDownresCoC; 
layout(location = 1) out vec4 outDownresColor; 
layout(location = 2) out vec4 outDownresFarMulColor; 

layout(set = 0, binding = 0) uniform sampler samplerLinear;
layout(set = 0, binding = 1) uniform sampler samplerPoint;

layout(set = 1, binding = 0) uniform texture2D TextureCoC;
layout(set = 1, binding = 1) uniform texture2D TextureColor;

void main()
{
    float cocNear = texture(sampler2D(TextureCoC, samplerPoint), inUV).r;
    vec4 color = texture(sampler2D(TextureColor, samplerPoint), inUV);
		
	vec2 pixelSize = 1.0f / textureSize(sampler2D(TextureColor, samplerPoint), 0);

	vec2 texCoord00 = inUV + vec2(-0.5f, -0.5f) * pixelSize;
	vec2 texCoord10 = inUV + vec2( 0.5f, -0.5f) * pixelSize;
	vec2 texCoord01 = inUV + vec2(-0.5f,  0.5f) * pixelSize;
	vec2 texCoord11 = inUV + vec2( 0.5f,  0.5f) * pixelSize;
	
	float cocFar00 = texture(sampler2D(TextureCoC, samplerPoint), texCoord00).g;
	float cocFar10 = texture(sampler2D(TextureCoC, samplerPoint), texCoord10).g;
	float cocFar01 = texture(sampler2D(TextureCoC, samplerPoint), texCoord01).g;
	float cocFar11 = texture(sampler2D(TextureCoC, samplerPoint), texCoord11).g;

	// Closer The Samples -> Bigger Contributions

	float weight00 = 1000.0f;
	vec4 colorMulCOCFar = weight00 * texture(sampler2D(TextureColor, samplerPoint), texCoord00);
	float weightsSum = weight00;
	
	float weight10 = 1.0f / (abs(cocFar00 - cocFar10) + 0.001f);
	colorMulCOCFar += weight10 * texture(sampler2D(TextureColor, samplerPoint), texCoord10);
	weightsSum += weight10;
	
	float weight01 = 1.0f / (abs(cocFar00 - cocFar01) + 0.001f);
	colorMulCOCFar += weight01 * texture(sampler2D(TextureColor, samplerPoint), texCoord01);
	weightsSum += weight01;
	
	float weight11 = 1.0f / (abs(cocFar00 - cocFar11) + 0.001f);
	colorMulCOCFar += weight11 * texture(sampler2D(TextureColor, samplerPoint), texCoord11);
	weightsSum += weight11;

	colorMulCOCFar /= weightsSum;
	colorMulCOCFar *= cocFar00;
	
    outDownresCoC = vec2(cocNear, cocFar00);
    outDownresColor = color;
    outDownresFarMulColor = colorMulCOCFar;
}
