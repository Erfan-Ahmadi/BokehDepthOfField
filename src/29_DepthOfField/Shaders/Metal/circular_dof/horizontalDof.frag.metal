#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_UniformDOF
{
    float maxRadius;
    float blend;
};

constant float4 _162[17] = { float4(0.014096000231802463531494140625, -0.02265799976885318756103515625, 0.055991001427173614501953125, 0.0044129998423159122467041015625), float4(-0.02061199955642223358154296875, -0.02557400055229663848876953125, 0.01918799988925457000732421875, 0.0), float4(-0.0387080013751983642578125, 0.00695700012147426605224609375, 0.0, 0.049222998321056365966796875), float4(-0.0214489996433258056640625, 0.040467999875545501708984375, 0.01830100081861019134521484375, 0.099928997457027435302734375), float4(0.01301500014960765838623046875, 0.050223000347614288330078125, 0.0548450015485286712646484375, 0.114688999950885772705078125), float4(0.0421780012547969818115234375, 0.038584999740123748779296875, 0.085768997669219970703125, 0.097079999744892120361328125), float4(0.05797199904918670654296875, 0.01981200091540813446044921875, 0.102517001330852508544921875, 0.0686739981174468994140625), float4(0.0636470019817352294921875, 0.0052519999444484710693359375, 0.108534999191761016845703125, 0.0466430000960826873779296875), float4(0.064754001796245574951171875, 0.0, 0.109709002077579498291015625, 0.0386970005929470062255859375), float4(0.0636470019817352294921875, 0.0052519999444484710693359375, 0.108534999191761016845703125, 0.0466430000960826873779296875), float4(0.05797199904918670654296875, 0.01981200091540813446044921875, 0.102517001330852508544921875, 0.0686739981174468994140625), float4(0.0421780012547969818115234375, 0.038584999740123748779296875, 0.085768997669219970703125, 0.097079999744892120361328125), float4(0.01301500014960765838623046875, 0.050223000347614288330078125, 0.0548450015485286712646484375, 0.114688999950885772705078125), float4(-0.0214489996433258056640625, 0.040467999875545501708984375, 0.01830100081861019134521484375, 0.099928997457027435302734375), float4(-0.0387080013751983642578125, 0.00695700012147426605224609375, 0.0, 0.049222998321056365966796875), float4(-0.02061199955642223358154296875, -0.02557400055229663848876953125, 0.01918799988925457000732421875, 0.0), float4(0.014096000231802463531494140625, -0.02265799976885318756103515625, 0.055991001427173614501953125, 0.0044129998423159122467041015625) };
constant float4 _172[17] = { float4(0.0001150000025518238544464111328125, 0.00911599956452846527099609375, 0.0, 0.05114699900150299072265625), float4(0.0053240000270307064056396484375, 0.013415999710559844970703125, 0.00931099988520145416259765625, 0.07527600228786468505859375), float4(0.013752999715507030487060546875, 0.01651900075376033782958984375, 0.02437599934637546539306640625, 0.09268499910831451416015625), float4(0.02470000088214874267578125, 0.01721500046551227569580078125, 0.043940000236034393310546875, 0.096591003239154815673828125), float4(0.036692999303340911865234375, 0.015064000152051448822021484375, 0.0653750002384185791015625, 0.084521003067493438720703125), float4(0.0479759983718395233154296875, 0.01068400032818317413330078125, 0.08553899824619293212890625, 0.0599480010569095611572265625), float4(0.057015001773834228515625, 0.00557000003755092620849609375, 0.101695001125335693359375, 0.031254000961780548095703125), float4(0.062781997025012969970703125, 0.00152900000102818012237548828125, 0.11200200021266937255859375, 0.008577999658882617950439453125), float4(0.064754001796245574951171875, 0.0, 0.115525998175144195556640625, 0.0), float4(0.062781997025012969970703125, 0.00152900000102818012237548828125, 0.11200200021266937255859375, 0.008577999658882617950439453125), float4(0.057015001773834228515625, 0.00557000003755092620849609375, 0.101695001125335693359375, 0.031254000961780548095703125), float4(0.0479759983718395233154296875, 0.01068400032818317413330078125, 0.08553899824619293212890625, 0.0599480010569095611572265625), float4(0.036692999303340911865234375, 0.015064000152051448822021484375, 0.0653750002384185791015625, 0.084521003067493438720703125), float4(0.02470000088214874267578125, 0.01721500046551227569580078125, 0.043940000236034393310546875, 0.096591003239154815673828125), float4(0.013752999715507030487060546875, 0.01651900075376033782958984375, 0.02437599934637546539306640625, 0.09268499910831451416015625), float4(0.0053240000270307064056396484375, 0.013415999710559844970703125, 0.00931099988520145416259765625, 0.07527600228786468505859375), float4(0.0001150000025518238544464111328125, 0.00911599956452846527099609375, 0.0, 0.05114699900150299072265625) };
constant float4 _182[17] = { float4(-0.00144200003705918788909912109375, 0.02665599994361400604248046875, 0.0, 0.085608996450901031494140625), float4(0.0104879997670650482177734375, 0.0309449993073940277099609375, 0.017733000218868255615234375, 0.099384002387523651123046875), float4(0.0237709991633892059326171875, 0.0308299995958805084228515625, 0.0374750010669231414794921875, 0.09901200234889984130859375), float4(0.0363559983670711517333984375, 0.0267699994146823883056640625, 0.05618099868297576904296875, 0.085975997149944305419921875), float4(0.0468220002949237823486328125, 0.020139999687671661376953125, 0.071736998856067657470703125, 0.064680002629756927490234375), float4(0.054554998874664306640625, 0.012687000446021556854248046875, 0.0832310020923614501953125, 0.0407450012862682342529296875), float4(0.05960600078105926513671875, 0.0060740001499652862548828125, 0.090737998485565185546875, 0.01950700022280216217041015625), float4(0.062366001307964324951171875, 0.001583999954164028167724609375, 0.09484100341796875, 0.005086000077426433563232421875), float4(0.063231997191905975341796875, 0.0, 0.09612800180912017822265625, 0.0), float4(0.062366001307964324951171875, 0.001583999954164028167724609375, 0.09484100341796875, 0.005086000077426433563232421875), float4(0.05960600078105926513671875, 0.0060740001499652862548828125, 0.090737998485565185546875, 0.01950700022280216217041015625), float4(0.054554998874664306640625, 0.012687000446021556854248046875, 0.0832310020923614501953125, 0.0407450012862682342529296875), float4(0.0468220002949237823486328125, 0.020139999687671661376953125, 0.071736998856067657470703125, 0.064680002629756927490234375), float4(0.0363559983670711517333984375, 0.0267699994146823883056640625, 0.05618099868297576904296875, 0.085975997149944305419921875), float4(0.0237709991633892059326171875, 0.0308299995958805084228515625, 0.0374750010669231414794921875, 0.09901200234889984130859375), float4(0.0104879997670650482177734375, 0.0309449993073940277099609375, 0.017733000218868255615234375, 0.099384002387523651123046875), float4(-0.00144200003705918788909912109375, 0.02665599994361400604248046875, 0.0, 0.085608996450901031494140625) };

struct main0_out
{
    float4 out_var_SV_TARGET [[color(0)]];
    float4 out_var_SV_TARGET1 [[color(1)]];
    float4 out_var_SV_TARGET2 [[color(2)]];
    float4 out_var_SV_TARGET3 [[color(3)]];
    float4 out_var_SV_TARGET4 [[color(4)]];
    float4 out_var_SV_TARGET5 [[color(5)]];
    float4 out_var_SV_TARGET6 [[color(6)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

struct FsData
{
    sampler samplerLinear [[id(0)]];
    sampler samplerPoint [[id(1)]];
};

struct FsDataPerFrame
{
    texture2d<float> TextureColor           [[id(0)]];
    texture2d<float> TextureCoC             [[id(1)]];
    texture2d<float> TextureNearCoC         [[id(2)]];
    texture2d<float> TextureColorMulFar     [[id(3)]];
    constant type_UniformDOF& UniformDOF    [[id(4)]];
};

fragment main0_out stageMain(
    main0_in in [[stage_in]],
    constant FsData&         fsData         [[buffer(UPDATE_FREQ_NONE)]],
    constant FsDataPerFrame& fsDataPerFrame [[buffer(UPDATE_FREQ_PER_FRAME)]])
{
    main0_out out = {};
    uint2 _189 = uint2(fsDataPerFrame.TextureColor.get_width(), fsDataPerFrame.TextureColor.get_height());
    float4 _199 = fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, in.in_var_TEXCOORD0);
    float _200 = _199.y;
    float4 _203 = fsDataPerFrame.TextureNearCoC.sample(fsData.samplerPoint, in.in_var_TEXCOORD0);
    float4 _275;
    float4 _276;
    float4 _277;
    float _278;
    if (_200 > 0.0)
    {
        float4 _209;
        float4 _212;
        float4 _214;
        float _216;
        int _218;
        _209 = float4(0.0);
        _212 = float4(0.0);
        _214 = float4(0.0);
        _216 = 0.0;
        _218 = 0;
        uint _220;
        for (;;)
        {
            _220 = uint(_218);
            if (_220 <= 16u)
            {
                int _224 = int(_220 - 8u);
                float2 _231 = in.in_var_TEXCOORD0 + (((float2(1.0) / float2(float(_189.x), float(_189.y))) * float2(float(_224), 0.0)) * fsDataPerFrame.UniformDOF.maxRadius);
                uint _233 = uint(_224) + 8u;
                float4 _241 = fsDataPerFrame.TextureCoC.sample(fsData.samplerPoint, _231);
                float _242 = _241.y;
                bool _243 = _242 == 0.0;
                float4 _249 = fsDataPerFrame.TextureColorMulFar.sample(fsData.samplerPoint, select(_231, in.in_var_TEXCOORD0, bool2(_243)));
                float _250 = _249.x;
                float _258 = _249.y;
                float _266 = _249.z;
                _209 += float4(_162[_233].xy * _266, _172[_233].xy * _266);
                _212 += float4(_162[_233].xy * _258, _172[_233].xy * _258);
                _214 += float4(_162[_233].xy * _250, _172[_233].xy * _250);
                _216 += (_243 ? _200 : _242);
                _218++;
                continue;
            }
            else
            {
                break;
            }
        }
        _275 = _214;
        _276 = _212;
        _277 = _209;
        _278 = _216 * 0.0588235296308994293212890625;
    }
    else
    {
        _275 = float4(0.0);
        _276 = float4(0.0);
        _277 = float4(0.0);
        _278 = 0.0;
    }
    float2 _324;
    float2 _325;
    float2 _326;
    if (_203.x > 0.0)
    {
        float2 _283;
        float2 _286;
        float2 _288;
        int _290;
        _283 = float2(0.0);
        _286 = float2(0.0);
        _288 = float2(0.0);
        _290 = 0;
        uint _292;
        for (;;)
        {
            _292 = uint(_290);
            if (_292 <= 16u)
            {
                int _296 = int(_292 - 8u);
                float2 _303 = in.in_var_TEXCOORD0 + (((float2(1.0) / float2(float(_189.x), float(_189.y))) * float2(float(_296), 0.0)) * fsDataPerFrame.UniformDOF.maxRadius);
                float4 _317 = fsDataPerFrame.TextureColor.sample(fsData.samplerLinear, select(_303, in.in_var_TEXCOORD0, bool2(fsDataPerFrame.TextureNearCoC.sample(fsData.samplerPoint, _303).x == 0.0)));
                _283 += (_182[uint(_296) + 8u].xy * _317.z);
                _286 += (_182[uint(_296) + 8u].xy * _317.y);
                _288 += (_182[uint(_296) + 8u].xy * _317.x);
                _290++;
                continue;
            }
            else
            {
                break;
            }
        }
        _324 = _288;
        _325 = _286;
        _326 = _283;
    }
    else
    {
        _324 = float2(0.0);
        _325 = float2(0.0);
        _326 = float2(0.0);
    }
    out.out_var_SV_TARGET = _275;
    out.out_var_SV_TARGET1 = _276;
    out.out_var_SV_TARGET2 = _277;
    out.out_var_SV_TARGET3 = float4(_324, 0, 0);
    out.out_var_SV_TARGET4 = float4(_325, 0, 0);
    out.out_var_SV_TARGET5 = float4(_326, 0, 0);
    out.out_var_SV_TARGET6 = float4(_278, 0, 0, 0);
    return out;
}