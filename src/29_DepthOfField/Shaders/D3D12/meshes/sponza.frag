struct PointLight 
{
	float3 position;
	float3 ambient;
	float3 diffuse;
	float3 specular;
    float3 att_params;
	float _pad0;
};

cbuffer PointLightsData : register(b3, UPDATE_FREQ_PER_FRAME)
{
	PointLight PointLights[NUM_LIGHTS];
};

cbuffer cbTextureRootConstants : register(b2) 
{
	uint albedoMap;
}

struct PsIn
{    
    float3 normal	: TEXCOORD0;
	float3 pos		: TEXCOORD1;
	float2 uv		: TEXCOORD2;
    float4 position : SV_Position;
};

SamplerState	samplerAnisotropic		: register(s0);

// material parameters
Texture2D						textureMaps[]		: register(t3);

float3 calculatePointLight(PointLight pointLight, float3 normal, float3 fragPosition, float3 albedo);

struct PSOut
{
    float4 color	: SV_Target0;
};

PSOut main(PsIn input) : SV_TARGET
{
	PSOut output;

	float3 normal = normalize(input.normal.xyz);
	
	float3 result = float3(0, 0, 0);
	
	float3 albedo = textureMaps[albedoMap].Sample(samplerAnisotropic, input.uv).rgb;

	for(int i = 0; i < NUM_LIGHTS; ++i)
		result += calculatePointLight(PointLights[i], normal, input.pos.xyz, albedo);

    output.color = float4(result, 1.0f);

	return output;
}

float3 calculatePointLight(PointLight pointLight, float3 normal, float3 fragPosition, float3 albedo)
{
	float3 difference = pointLight.position - fragPosition;

	float3 lightDir = normalize(difference);
	float3 reflectDir = reflect(-lightDir, normal);

	// Calc Attenuation
	float distance = length(difference);
	float3 dis = float3(1, distance, pow(distance, 2));
	float attenuation = 1.0f / dot(dis, pointLight.att_params);

	// Ambient
    float3 ambient = pointLight.ambient;

	// Diffuse
	float diff = max(dot(lightDir, normal), 0.0f);
	float3 diffuse = diff * pointLight.diffuse;

	return attenuation * (ambient + diffuse) * albedo;
}