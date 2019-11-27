#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct main0_in
{
    float3 in_var_COLOR [[user(locn0)]];
};

fragment main0_out stageMain(main0_in in [[stage_in]])
{
    main0_out out = {};
	out.out_var_SV_TARGET = float4(in.in_var_COLOR, 1.0);
    return out;
}

