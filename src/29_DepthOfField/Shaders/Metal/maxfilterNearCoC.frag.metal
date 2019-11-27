#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#define RADIUS 6

struct main0_out
{
    float out_var_SV_TARGET [[color(0)]];
};

struct main0_in
{
    float2 UV [[user(locn0)]];
};

struct FsData
{
    sampler samplerLinear   [[id(0)]];
};

struct FsDataPerFrame
{
    texture2d<float> NearCoCTexture [[id(0)]];
};

fragment float4 stageMain(
    main0_in in [[stage_in]],
    constant FsData&         fsData         [[buffer(UPDATE_FREQ_NONE)]],
    constant FsDataPerFrame& fsDataPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    main0_out out = {};

    float2 step = 1.0f / float2(fsDataPerFrame.NearCoCTexture.get_width(), fsDataPerFrame.NearCoCTexture.get_height());

    float max = 0;

    for (int i = 0; i <= RADIUS * 2; ++i)
    {
        int index = (i - RADIUS);

		float2 coords;

#ifdef HORIZONTAL
			coords = in.UV + step * float2(float(index), 0.0);
#else
			coords = in.UV + step * float2(0.0, float(index));
#endif

        max = fast::max(max, fsDataPerFrame.NearCoCTexture.sample(fsData.samplerLinear, coords).x);
    }

    out.out_var_SV_TARGET = max;
    return float4(out.out_var_SV_TARGET, 0, 0, 0);
}
