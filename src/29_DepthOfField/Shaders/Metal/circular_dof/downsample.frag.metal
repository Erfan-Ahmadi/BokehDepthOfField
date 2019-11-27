#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_TARGET [[color(0)]];
    float4 out_var_SV_TARGET1 [[color(1)]];
    float4 out_var_SV_TARGET2 [[color(2)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

struct FsData
{
    sampler samplerPoint [[id(0)]];
};

struct FsDataPerFrame
{
	texture2d<float> TextureCoC     [[id(0)]];
	texture2d<float> TextureColor   [[id(1)]];
};

fragment main0_out stageMain(
	main0_in in [[stage_in]],
    constant FsData&         fsData         [[buffer(UPDATE_FREQ_NONE)]],
    constant FsDataPerFrame& fsDataPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    main0_out out = {};
    uint2 _50 = uint2(fsDataPerFrame.TextureColor.get_width(), fsDataPerFrame.TextureColor.get_height());
    float2 _55 = float2(float(_50.x), float(_50.y));
    float2 _57 = in.in_var_TEXCOORD0 + (float2(-0.5) / _55);
    float2 _59 = in.in_var_TEXCOORD0 + (float2(0.5, -0.5) / _55);
    float2 _61 = in.in_var_TEXCOORD0 + (float2(-0.5, 0.5) / _55);
    float2 _63 = in.in_var_TEXCOORD0 + (float2(0.5) / _55);
    float _68 = fast::clamp(fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, _57).y * 5.0, 0.0, 1.0);
    float _90 = 1.0 / (abs(_68 - fast::clamp(fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, _59).y * 5.0, 0.0, 1.0)) + 0.001000000047497451305389404296875);
    float _99 = 1.0 / (abs(_68 - fast::clamp(fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, _61).y * 5.0, 0.0, 1.0)) + 0.001000000047497451305389404296875);
    float _108 = 1.0 / (abs(_68 - fast::clamp(fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, _63).y * 5.0, 0.0, 1.0)) + 0.001000000047497451305389404296875);
    out.out_var_SV_TARGET = float4(fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, in.in_var_TEXCOORD0).x, _68, 0, 0);
    out.out_var_SV_TARGET1 = fsDataPerFrame.TextureColor.sample(fsData.samplerPoint, in.in_var_TEXCOORD0);
    out.out_var_SV_TARGET2 = (((((fsDataPerFrame.TextureColor.sample(fsData.samplerPoint, _57) * 1000.0) + (fsDataPerFrame.TextureColor.sample(fsData.samplerPoint, _59) * _90)) + (fsDataPerFrame.TextureColor.sample(fsData.samplerPoint, _61) * _99)) + (fsDataPerFrame.TextureColor.sample(fsData.samplerPoint, _63) * _108)) / float4(((1000.0 + _90) + _99) + _108)) * _68;
    return out;
}

