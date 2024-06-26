#include <metal_stdlib>
#include <metal_atomic>
using namespace metal;

inline float3x3 matrix_ctor(float4x4 m) {
        return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}

struct Fragment_Shader
{
    struct PointLight
    {
        float3 position;
        float3 ambient;
        float3 diffuse;
        float3 specular;
        float3 att_params;
    };

    struct Uniforms_PointLightsData
    {
        array<PointLight, NUM_LIGHTS> PointLights;
    };

    constant Uniforms_PointLightsData& PointLightsData;
    struct Uniforms_cbTextureRootConstants
    {
        uint albedoMap;
    };
    constant Uniforms_cbTextureRootConstants& cbTextureRootConstants;

    struct PsIn
    {
        float4 position  [[position]];
        float3 normal;
        float3 pos;
        float2 uv;
    };

    sampler samplerAnisotropic;

    struct Uniforms_textureMaps {
         array<texture2d<float>, 128> Textures;
    };
    constant texture2d<float>* textureMaps;
    
    struct PSOut
    {
        float4 color [[color(0)]];
    };

    float3 calculatePointLight(PointLight pointLight, float3 normal, float3 fragPosition, float3 albedo)
    {
        float3 difference = pointLight.position - fragPosition;

        float3 lightDir = normalize(difference);

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

    PSOut main(PsIn input)
    {
        PSOut output;
        float3 normal = normalize(((input).normal).xyz);
        float3 result = float3((float)0, (float)0, (float)0);
        float3 albedo = (textureMaps[cbTextureRootConstants.albedoMap].sample(samplerAnisotropic, (input).uv)).rgb;
        for (int i = 0; (i < NUM_LIGHTS); (++i))
        {
            (result += calculatePointLight(PointLightsData.PointLights[i], normal, ((input).pos).xyz, albedo));
        }
        ((output).color = float4(result, 1.0));
        return output;
    };
    Fragment_Shader(
        constant Uniforms_PointLightsData& PointLightsData,
        constant Uniforms_cbTextureRootConstants& cbTextureRootConstants,
        sampler samplerAnisotropic,
        constant texture2d<float>* textureMaps) :
        
        PointLightsData(PointLightsData),
        cbTextureRootConstants(cbTextureRootConstants),
        samplerAnisotropic(samplerAnisotropic),
        textureMaps(textureMaps)
    {}
};

struct FSData
{
    sampler samplerAnisotropic [[id(1)]];
    texture2d<float> textureMaps[128];
};

struct FSDataPerFrame {
    constant Fragment_Shader::Uniforms_PointLightsData& PointLightsData [[id(3)]];
};

fragment Fragment_Shader::PSOut stageMain(
    Fragment_Shader::PsIn inputData             [[stage_in]],
    constant FSData& fsData                     [[buffer(UPDATE_FREQ_NONE)]],
    constant FSDataPerFrame& fsDataPerFrame     [[buffer(UPDATE_FREQ_PER_FRAME)]],
    constant Fragment_Shader::Uniforms_cbTextureRootConstants& cbTextureRootConstants [[buffer(UPDATE_FREQ_USER)]]
)
{
    Fragment_Shader::PsIn input0;
    input0.normal = inputData.normal;
    input0.pos = inputData.pos;
    input0.uv = inputData.uv;
    input0.position = inputData.position;
    Fragment_Shader main(fsDataPerFrame.PointLightsData, cbTextureRootConstants, fsData.samplerAnisotropic, fsData.textureMaps);
    Fragment_Shader::PSOut result = main.main(input0);
    Fragment_Shader::PSOut output;
    output.color = result.color;
    return output;
}
