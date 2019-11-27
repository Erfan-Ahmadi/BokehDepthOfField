#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_cbPerPass
{
    float4x4 proj;
    float4x4 view;
};

struct main0_out
{
    float3 out_var_COLOR [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float3 in_var_NORMAL [[attribute(1)]];
    float3 in_var_COLOR [[attribute(2)]];
};

struct VSDataPerFrame {
    constant type_cbPerPass& cbPerPass [[id(1)]];
};

vertex main0_out stageMain(
    main0_in in [[stage_in]],
    constant VSDataPerFrame& vsDataPerFrame     [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    main0_out out = {};
    out.gl_Position =  vsDataPerFrame.cbPerPass.proj * vsDataPerFrame.cbPerPass.view * float4(in.in_var_NORMAL + (in.in_var_POSITION * 0.5), 1.0);
    out.out_var_COLOR = in.in_var_COLOR;
    return out;
}

