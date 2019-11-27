#version 450 core

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inFragPos;

layout(location = 0) out vec4 outAlbedo;

struct PointLight 
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
    vec3 att_params;
	float _pad0;
};

layout(std140, UPDATE_FREQ_PER_FRAME, binding = 3) uniform PointLightsData
{
	uniform PointLight PointLights[NUM_LIGHTS];
};

layout(push_constant) uniform cbTextureRootConstantsData
{
	uint albedoMap;
} cbTextureRootConstants;

layout(UPDATE_FREQ_NONE, binding = 1) uniform texture2D textureMaps[TOTAL_IMGS];
layout(UPDATE_FREQ_NONE, binding = 2) uniform sampler samplerAnisotropic;

vec3 calculatePointLight(PointLight pointLight, vec3 normal, vec3 fragPosition, vec3 albedo);

void main()
{
	vec3 result = vec3(0, 0, 0);
	
	vec3 normal = normalize(inNormal);
	vec3 albedo = texture(sampler2D(textureMaps[cbTextureRootConstants.albedoMap], samplerAnisotropic), inUV).rgb;

	for(int i = 0; i < NUM_LIGHTS; ++i)
		result += calculatePointLight(PointLights[i], normal, inFragPos, albedo);

	outAlbedo = vec4(result, 1.0f);
}

vec3 calculatePointLight(PointLight pointLight, vec3 normal, vec3 fragPosition, vec3 albedo)
{
	vec3 difference = pointLight.position - fragPosition;

	vec3 lightDir = normalize(difference);
	vec3 reflectDir = reflect(-lightDir, normal);

	// Calc Attenuation
	float distance = length(difference);
	vec3 dis = vec3(1, distance, pow(distance, 2));
	float attenuation = 1.0f / dot(dis, pointLight.att_params);

	// Ambient
    vec3 ambient = pointLight.ambient;

	// Diffuse
	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * pointLight.diffuse;

	return attenuation * (ambient + diffuse) * albedo;
}