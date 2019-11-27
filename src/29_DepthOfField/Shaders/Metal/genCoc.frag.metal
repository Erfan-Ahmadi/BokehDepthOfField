#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_UniformDOF
{
    float maxRadius;
    float blend;
    float nb;
    float ne;
    float fb;
    float fe;
    float2 projParams;
};

struct main0_out
{
    float2 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

struct FsData
{
    sampler samplerPoint    [[id(1)]];
};

struct FsDataPerFrame
{
    texture2d<float> DepthTexture           [[id(0)]];
    constant type_UniformDOF& UniformDOF    [[id(1)]];
};

fragment float4 stageMain(
    main0_in in [[stage_in]],
    constant FsData&         fsData         [[buffer(UPDATE_FREQ_NONE)]],
    constant FsDataPerFrame& fsDataPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    main0_out out = {};
    float _55 = ((2.0 * zNear) * zFar) / ((zFar + zNear) - (((2.0 * fsDataPerFrame.DepthTexture.sample(fsData.samplerPoint, in.in_var_TEXCOORD0).x) - 1.0) * (zFar - zNear)));
    out.out_var_SV_Target = float2((fsDataPerFrame.UniformDOF.ne - _55) / (fsDataPerFrame.UniformDOF.ne - fsDataPerFrame.UniformDOF.nb), (_55 - fsDataPerFrame.UniformDOF.fb) / (fsDataPerFrame.UniformDOF.fe - fsDataPerFrame.UniformDOF.fb));
    return float4(out.out_var_SV_Target, 0, 0);
}
