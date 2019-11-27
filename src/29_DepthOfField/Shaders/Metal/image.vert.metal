#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float4 gl_Position [[position]];
};

vertex main0_out stageMain(uint gl_VertexIndex [[vertex_id]])
{
    main0_out out = {};
    float2 _29 = float2(float((gl_VertexIndex << 1u) & 2u), float(gl_VertexIndex & 2u));
    out.gl_Position = float4(((_29 * 2.0) - float2(1.0)) * float2(1.0, -1.0), 0.0, 1.0);
    out.out_var_TEXCOORD0 = _29;
    return out;
}
