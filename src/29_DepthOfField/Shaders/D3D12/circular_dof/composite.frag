static const uint KERNEL_RADIUS = 8;
static const uint KERNEL_COUNT = 17;

// 2-Component

static const float4 Kernel0BracketsRealXY_ImZW_2 = float4(-0.038708,0.943062,-0.025574,0.660892);
static const float2 Kernel0Weights_RealX_ImY_2 = float2(0.411259,-0.548794);
static const float4 Kernel0_RealX_ImY_RealZ_ImW_2[] = {
        float4(/*XY: Non Bracketed*/0.014096,-0.022658,/*Bracketed WZ:*/0.055991,0.004413),
        float4(/*XY: Non Bracketed*/-0.020612,-0.025574,/*Bracketed WZ:*/0.019188,0.000000),
        float4(/*XY: Non Bracketed*/-0.038708,0.006957,/*Bracketed WZ:*/0.000000,0.049223),
        float4(/*XY: Non Bracketed*/-0.021449,0.040468,/*Bracketed WZ:*/0.018301,0.099929),
        float4(/*XY: Non Bracketed*/0.013015,0.050223,/*Bracketed WZ:*/0.054845,0.114689),
        float4(/*XY: Non Bracketed*/0.042178,0.038585,/*Bracketed WZ:*/0.085769,0.097080),
        float4(/*XY: Non Bracketed*/0.057972,0.019812,/*Bracketed WZ:*/0.102517,0.068674),
        float4(/*XY: Non Bracketed*/0.063647,0.005252,/*Bracketed WZ:*/0.108535,0.046643),
        float4(/*XY: Non Bracketed*/0.064754,0.000000,/*Bracketed WZ:*/0.109709,0.038697),
        float4(/*XY: Non Bracketed*/0.063647,0.005252,/*Bracketed WZ:*/0.108535,0.046643),
        float4(/*XY: Non Bracketed*/0.057972,0.019812,/*Bracketed WZ:*/0.102517,0.068674),
        float4(/*XY: Non Bracketed*/0.042178,0.038585,/*Bracketed WZ:*/0.085769,0.097080),
        float4(/*XY: Non Bracketed*/0.013015,0.050223,/*Bracketed WZ:*/0.054845,0.114689),
        float4(/*XY: Non Bracketed*/-0.021449,0.040468,/*Bracketed WZ:*/0.018301,0.099929),
        float4(/*XY: Non Bracketed*/-0.038708,0.006957,/*Bracketed WZ:*/0.000000,0.049223),
        float4(/*XY: Non Bracketed*/-0.020612,-0.025574,/*Bracketed WZ:*/0.019188,0.000000),
        float4(/*XY: Non Bracketed*/0.014096,-0.022658,/*Bracketed WZ:*/0.055991,0.004413)
};
static const float4 Kernel1BracketsRealXY_ImZW_2 = float4(0.000115,0.559524,0.000000,0.178226);
static const float2 Kernel1Weights_RealX_ImY_2 = float2(0.513282,4.561110);
static const float4 Kernel1_RealX_ImY_RealZ_ImW_2[] = {
        float4(/*XY: Non Bracketed*/0.000115,0.009116,/*Bracketed WZ:*/0.000000,0.051147),
        float4(/*XY: Non Bracketed*/0.005324,0.013416,/*Bracketed WZ:*/0.009311,0.075276),
        float4(/*XY: Non Bracketed*/0.013753,0.016519,/*Bracketed WZ:*/0.024376,0.092685),
        float4(/*XY: Non Bracketed*/0.024700,0.017215,/*Bracketed WZ:*/0.043940,0.096591),
        float4(/*XY: Non Bracketed*/0.036693,0.015064,/*Bracketed WZ:*/0.065375,0.084521),
        float4(/*XY: Non Bracketed*/0.047976,0.010684,/*Bracketed WZ:*/0.085539,0.059948),
        float4(/*XY: Non Bracketed*/0.057015,0.005570,/*Bracketed WZ:*/0.101695,0.031254),
        float4(/*XY: Non Bracketed*/0.062782,0.001529,/*Bracketed WZ:*/0.112002,0.008578),
        float4(/*XY: Non Bracketed*/0.064754,0.000000,/*Bracketed WZ:*/0.115526,0.000000),
        float4(/*XY: Non Bracketed*/0.062782,0.001529,/*Bracketed WZ:*/0.112002,0.008578),
        float4(/*XY: Non Bracketed*/0.057015,0.005570,/*Bracketed WZ:*/0.101695,0.031254),
        float4(/*XY: Non Bracketed*/0.047976,0.010684,/*Bracketed WZ:*/0.085539,0.059948),
        float4(/*XY: Non Bracketed*/0.036693,0.015064,/*Bracketed WZ:*/0.065375,0.084521),
        float4(/*XY: Non Bracketed*/0.024700,0.017215,/*Bracketed WZ:*/0.043940,0.096591),
        float4(/*XY: Non Bracketed*/0.013753,0.016519,/*Bracketed WZ:*/0.024376,0.092685),
        float4(/*XY: Non Bracketed*/0.005324,0.013416,/*Bracketed WZ:*/0.009311,0.075276),
        float4(/*XY: Non Bracketed*/0.000115,0.009116,/*Bracketed WZ:*/0.000000,0.051147)
};

// 1-Component

static const float4 Kernel0BracketsRealXY_ImZW_1 = float4(-0.001442,0.672786,0.000000,0.311371);
static const float2 Kernel0Weights_RealX_ImY_1 = float2(0.767583,1.862321);
static const float4 Kernel0_RealX_ImY_RealZ_ImW_1[] = {
        float4(/*XY: Non Bracketed*/-0.001442,0.026656,/*Bracketed WZ:*/0.000000,0.085609),
        float4(/*XY: Non Bracketed*/0.010488,0.030945,/*Bracketed WZ:*/0.017733,0.099384),
        float4(/*XY: Non Bracketed*/0.023771,0.030830,/*Bracketed WZ:*/0.037475,0.099012),
        float4(/*XY: Non Bracketed*/0.036356,0.026770,/*Bracketed WZ:*/0.056181,0.085976),
        float4(/*XY: Non Bracketed*/0.046822,0.020140,/*Bracketed WZ:*/0.071737,0.064680),
        float4(/*XY: Non Bracketed*/0.054555,0.012687,/*Bracketed WZ:*/0.083231,0.040745),
        float4(/*XY: Non Bracketed*/0.059606,0.006074,/*Bracketed WZ:*/0.090738,0.019507),
        float4(/*XY: Non Bracketed*/0.062366,0.001584,/*Bracketed WZ:*/0.094841,0.005086),
        float4(/*XY: Non Bracketed*/0.063232,0.000000,/*Bracketed WZ:*/0.096128,0.000000),
        float4(/*XY: Non Bracketed*/0.062366,0.001584,/*Bracketed WZ:*/0.094841,0.005086),
        float4(/*XY: Non Bracketed*/0.059606,0.006074,/*Bracketed WZ:*/0.090738,0.019507),
        float4(/*XY: Non Bracketed*/0.054555,0.012687,/*Bracketed WZ:*/0.083231,0.040745),
        float4(/*XY: Non Bracketed*/0.046822,0.020140,/*Bracketed WZ:*/0.071737,0.064680),
        float4(/*XY: Non Bracketed*/0.036356,0.026770,/*Bracketed WZ:*/0.056181,0.085976),
        float4(/*XY: Non Bracketed*/0.023771,0.030830,/*Bracketed WZ:*/0.037475,0.099012),
        float4(/*XY: Non Bracketed*/0.010488,0.030945,/*Bracketed WZ:*/0.017733,0.099384),
        float4(/*XY: Non Bracketed*/-0.001442,0.026656,/*Bracketed WZ:*/0.000000,0.085609)
};


cbuffer UniformDOF : register(b0, UPDATE_FREQ_PER_FRAME)
{
	float	maxRadius;
	float	blend;
};

struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

SamplerState	samplerLinear			: register(s0);
SamplerState	samplerPoint				: register(s1);

Texture2D		TextureCoC			: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureFarR			: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureFarG			: register(t2, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureFarB			: register(t3, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearR			: register(t4, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearG			: register(t5, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearB			: register(t6, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureColor			: register(t7, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureNearCoC		: register(t8, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureWeights		: register(t9, UPDATE_FREQ_PER_FRAME);

float2 multComplex(float2 p, float2 q)
{
    return float2(p.x*q.x-p.y*q.y, p.x*q.y+p.y*q.x);
}

float4 main(VSOutput input) : SV_TARGET
{	
	uint w, h;
	TextureFarR.GetDimensions(w, h);
	float2 step = 1.0f / float2(w, h);

	float4 color = (TextureColor.Sample(samplerLinear, input.UV));
	
	float cocValueFar = TextureCoC.Sample(samplerPoint, input.UV).g;
	float cocValueNear = TextureNearCoC.Sample(samplerPoint, input.UV).r;

	float4 filteredColorFar = float4(0, 0, 0, 0);
	float4 filteredColorNear = float4(0, 0, 0, 0);

	if (cocValueFar > 0)
	{
		float4 valR = float4(0, 0, 0, 0);
		float4 valG = float4(0, 0, 0, 0);
		float4 valB = float4(0, 0, 0, 0);
	
		float weightValue = 0;

		for (int i = 0; i <= KERNEL_RADIUS * 2; ++i)
		{
			int index = i - KERNEL_RADIUS;
			float2 coords = input.UV + step * float2(0.0, float(index)) * maxRadius;
			
			float cocValueSample = TextureCoC.Sample(samplerPoint, coords).g;
			
			if(cocValueSample == 0)
			{
				coords = input.UV;
				cocValueSample = cocValueFar;
			}
			
			float weightValueSample = (TextureWeights.Sample(samplerLinear, coords).r); // Linear Sampling!
			weightValue += weightValueSample;

			float4 imageTexelR = TextureFarR.Sample(samplerLinear, coords);  
			float4 imageTexelG = TextureFarG.Sample(samplerLinear, coords);  
			float4 imageTexelB = TextureFarB.Sample(samplerLinear, coords);  
        
			float2 c0 = Kernel0_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;
			float2 c1 = Kernel1_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;

			valR.xy += multComplex(imageTexelR.xy, c0);
			valR.zw += multComplex(imageTexelR.zw, c1);

			valG.xy += multComplex(imageTexelG.xy, c0);
			valG.zw += multComplex(imageTexelG.zw, c1);

			valB.xy += multComplex(imageTexelB.xy, c0); 
			valB.zw += multComplex(imageTexelB.zw, c1);   
		}
		
		weightValue /= (KERNEL_COUNT);

		float redChannel   = dot(valR.xy, Kernel0Weights_RealX_ImY_2) + dot(valR.zw, Kernel1Weights_RealX_ImY_2);
		float greenChannel = dot(valG.xy, Kernel0Weights_RealX_ImY_2) + dot(valG.zw, Kernel1Weights_RealX_ImY_2);
		float blueChannel  = dot(valB.xy, Kernel0Weights_RealX_ImY_2) + dot(valB.zw, Kernel1Weights_RealX_ImY_2);
		filteredColorFar = float4((float3(redChannel, greenChannel, blueChannel) / weightValue), 0);
	}
	
	if (cocValueNear > 0)
	{	
		float2 valR = float2(0, 0);
		float2 valG = float2(0, 0);
		float2 valB = float2(0, 0);
	
		for (int i = 0; i <= KERNEL_RADIUS * 2; ++i)
		{
			int index = i - KERNEL_RADIUS;
			float2 coords = input.UV + step * float2(0.0, float(index)) * maxRadius;
				
			float cocValueSample = TextureNearCoC.Sample(samplerPoint, coords).r;	
			if (cocValueSample == 0)
			{
				coords = input.UV;
				cocValueSample = cocValueNear;
			}

			float4 imageTexelR = TextureNearR.Sample(samplerLinear, coords);  
			float4 imageTexelG = TextureNearG.Sample(samplerLinear, coords);  
			float4 imageTexelB = TextureNearB.Sample(samplerLinear, coords);  
        
			float2 c0 = Kernel0_RealX_ImY_RealZ_ImW_1[index + KERNEL_RADIUS].xy;

			valR += multComplex(imageTexelR.xy, c0);
			valG += multComplex(imageTexelG.xy, c0);
			valB += multComplex(imageTexelB.xy, c0);  
		}
	
		float redChannel   = dot(valR.xy, Kernel0Weights_RealX_ImY_1);
		float greenChannel = dot(valG.xy, Kernel0Weights_RealX_ImY_1);
		float blueChannel  = dot(valB.xy, Kernel0Weights_RealX_ImY_1);
		filteredColorNear = float4(float3(redChannel, greenChannel, blueChannel), 0);
	}
	
	color = lerp(color, filteredColorFar,	saturate(cocValueFar * blend));
	color = lerp(color, filteredColorNear,	saturate(cocValueNear * blend));

	return color;
}