struct PointLight 
{
	float3 position;
	float3 ambient;
	float3 diffuse;
	float3 specular;
	
    float att_constant;
    float att_linear;
    float att_quadratic;
	float _pad0;
};

cbuffer cbTextureRootConstants : register(b2) 
{
	uint albedoMap;
}

cbuffer LightData : register(b1, UPDATE_FREQ_PER_FRAME)
{
	int numPointLights;
	float3 viewPos;
};

struct PsIn
{    
    float3 normal	: TEXCOORD0;
	float3 pos		: TEXCOORD1;
	float2 uv		: TEXCOORD2;
    float4 position : SV_Position;
};

SamplerState	samplerLinear		: register(s0);

StructuredBuffer <PointLight>	PointLights			: register(t0, UPDATE_FREQ_PER_FRAME);

// material parameters
Texture2D						textureMaps[]		: register(t3);

float3 calculatePointLight(PointLight pointLight, float3 normal, float3 viewDir, float3 fragPosition, float3 albedo);

struct PSOut
{
    float4 color	: SV_Target0;
};

PSOut main(PsIn input) : SV_TARGET
{
	PSOut output;

	float3 normal = normalize(input.normal.xyz);
	float3 viewDir = normalize(viewPos - input.pos.xyz);
	
	float3 result;
	
	float3 albedo = textureMaps[albedoMap].Sample(samplerLinear, input.uv).rgb;

	for(int i = 0; i < numPointLights; ++i)
		result += calculatePointLight(PointLights[i], normal, viewDir, input.pos.xyz, albedo);
		
    output.color = float4(result, 1.0f);

	return output;
}

float3 calculatePointLight(PointLight pointLight, float3 normal, float3 viewDir, float3 fragPosition, float3 albedo)
{
	float3 difference = pointLight.position - fragPosition;

	float3 lightDir = normalize(difference);
	float3 reflectDir = reflect(-lightDir, normal);

	// Calc Attenuation
	float distance = length(difference);
	float3 dis = float3(1, distance, pow(distance, 2));
	float3 att = float3(pointLight.att_constant, pointLight.att_linear, pointLight.att_quadratic);
	float attenuation = 1.0f / dot(dis, att);

	// Ambient
    float3 ambient = pointLight.ambient;

	// Diffuse
	float diff = max(dot(lightDir, normal), 0.0f);
	float3 diffuse = diff * pointLight.diffuse;

	// Specular
	float spec = pow(max(dot(reflectDir, viewDir), 0.0), 64);
	float3 specular = spec * pointLight.specular;  

	return attenuation * (ambient + diffuse + specular) * albedo;
}