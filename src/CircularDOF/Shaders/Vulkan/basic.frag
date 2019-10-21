#version 450 core

layout(push_constant) uniform cbTextureRootConstantsData
{
	uint albedoMap;
} cbTextureRootConstants;

layout(UPDATE_FREQ_NONE, binding = 6) uniform texture2D textureMaps[TOTAL_IMGS];
layout(UPDATE_FREQ_NONE, binding = 7) uniform sampler samplerLinear;

//SamplerState samplerLinear : register(s2);
//
//// material parameters
//Texture2D textureMaps[] : register(t3, UPDATE_FREQ_PER_FRAME);

layout(location = 0) in vec3 InNormal;
layout(location = 1) in vec2 InUV;

layout(location = 0) out vec4 OutAlbedo;

void main()
{	
	//load albedo
	vec3 albedo = texture(sampler2D(textureMaps[cbTextureRootConstants.albedoMap], samplerLinear), InUV).rgb;

	OutAlbedo = vec4(albedo, 1);
}