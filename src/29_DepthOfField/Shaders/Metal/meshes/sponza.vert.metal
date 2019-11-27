#include <metal_stdlib>
#include <metal_atomic>
using namespace metal;

inline float3x3 matrix_ctor(float4x4 m) {
        return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}
struct Vertex_Shader
{
	struct VsIn
	{
		float3 position;
		float3 normal;
		float2 texCoord;
	};

    struct VsIn_Pos
    {
        packed_float3 position;
    };
	struct VsIn_Norm
	{
		packed_float3 normal;
	};
	struct VsIn_Uv
	{
		packed_float2 texCoord;
	};

    struct Uniforms_cbPerPass
    {
        float4x4 proj;
        float4x4 view;
    };
    constant Uniforms_cbPerPass& cbPerPass;

    struct Uniforms_cbPerProp
    {
        float4x4 world;
    };
    constant Uniforms_cbPerProp& cbPerProp;

    struct PsIn
    {
        float4 position [[position]];
        float3 normal;
        float3 pos;
        float2 uv;
    };

	PsIn main(Vertex_Shader::VsIn In)
    {
		Vertex_Shader::PsIn Out;

        Out.position = ((cbPerPass.proj * cbPerPass.view)*(((cbPerProp.world)*(float4(In.position.xyz, 1.0)))));
        Out.normal = (((matrix_ctor(cbPerProp.world))*(In.normal.xyz))).xyz;

        Out.pos = (((cbPerProp.world)*(float4(In.position.xyz, 1.0)))).xyz;
        Out.uv = In.texCoord.xy;

        return Out;
    };

    Vertex_Shader(constant Uniforms_cbPerPass & cbPerPass,
    constant Uniforms_cbPerProp & cbPerProp) :
        cbPerPass(cbPerPass),
        cbPerProp(cbPerProp) {}
};

struct VSData {
    constant Vertex_Shader::Uniforms_cbPerProp& cbPerProp [[id(0)]];
};

struct VSDataPerFrame {
    constant Vertex_Shader::Uniforms_cbPerPass& cbPerPass [[id(1)]];
};

vertex Vertex_Shader::PsIn stageMain(
    uint vid [[vertex_id]],
    constant Vertex_Shader::VsIn_Pos * InPos    [[buffer(0)]],
    constant Vertex_Shader::VsIn_Norm * InNorm  [[buffer(1)]],
    constant Vertex_Shader::VsIn_Uv * InUv      [[buffer(2)]],
    constant VSData& vsData                     [[buffer(UPDATE_FREQ_NONE)]],
    constant VSDataPerFrame& vsDataPerFrame     [[buffer(UPDATE_FREQ_PER_FRAME)]]
)
{
    Vertex_Shader::VsIn In0;
    In0.position = InPos[vid].position;
    In0.normal = InNorm[vid].normal;
    In0.texCoord = InUv[vid].texCoord;
    Vertex_Shader main(vsDataPerFrame.cbPerPass, vsData.cbPerProp);
    return main.main(In0);
}
