#version 450 core

const int KERNEL_RADIUS = 8;
const int KERNEL_COUNT = 17;

//2-Components Filter

const vec4 Kernel0BracketsRealXY_ImZW_2 = vec4(-0.038708,0.943062,-0.025574,0.660892);
const vec2 Kernel0Weights_RealX_ImY_2 = vec2(0.411259,-0.548794);
const vec4 Kernel0_RealX_ImY_RealZ_ImW_2[] = vec4[](
        vec4(/*XY: Non Bracketed*/0.014096,-0.022658,/*Bracketed WZ:*/0.055991,0.004413),
        vec4(/*XY: Non Bracketed*/-0.020612,-0.025574,/*Bracketed WZ:*/0.019188,0.000000),
        vec4(/*XY: Non Bracketed*/-0.038708,0.006957,/*Bracketed WZ:*/0.000000,0.049223),
        vec4(/*XY: Non Bracketed*/-0.021449,0.040468,/*Bracketed WZ:*/0.018301,0.099929),
        vec4(/*XY: Non Bracketed*/0.013015,0.050223,/*Bracketed WZ:*/0.054845,0.114689),
        vec4(/*XY: Non Bracketed*/0.042178,0.038585,/*Bracketed WZ:*/0.085769,0.097080),
        vec4(/*XY: Non Bracketed*/0.057972,0.019812,/*Bracketed WZ:*/0.102517,0.068674),
        vec4(/*XY: Non Bracketed*/0.063647,0.005252,/*Bracketed WZ:*/0.108535,0.046643),
        vec4(/*XY: Non Bracketed*/0.064754,0.000000,/*Bracketed WZ:*/0.109709,0.038697),
        vec4(/*XY: Non Bracketed*/0.063647,0.005252,/*Bracketed WZ:*/0.108535,0.046643),
        vec4(/*XY: Non Bracketed*/0.057972,0.019812,/*Bracketed WZ:*/0.102517,0.068674),
        vec4(/*XY: Non Bracketed*/0.042178,0.038585,/*Bracketed WZ:*/0.085769,0.097080),
        vec4(/*XY: Non Bracketed*/0.013015,0.050223,/*Bracketed WZ:*/0.054845,0.114689),
        vec4(/*XY: Non Bracketed*/-0.021449,0.040468,/*Bracketed WZ:*/0.018301,0.099929),
        vec4(/*XY: Non Bracketed*/-0.038708,0.006957,/*Bracketed WZ:*/0.000000,0.049223),
        vec4(/*XY: Non Bracketed*/-0.020612,-0.025574,/*Bracketed WZ:*/0.019188,0.000000),
        vec4(/*XY: Non Bracketed*/0.014096,-0.022658,/*Bracketed WZ:*/0.055991,0.004413)
);
const vec4 Kernel1BracketsRealXY_ImZW_2 = vec4(0.000115,0.559524,0.000000,0.178226);
const vec2 Kernel1Weights_RealX_ImY_2 = vec2(0.513282,4.561110);
const vec4 Kernel1_RealX_ImY_RealZ_ImW_2[] = vec4[](
        vec4(/*XY: Non Bracketed*/0.000115,0.009116,/*Bracketed WZ:*/0.000000,0.051147),
        vec4(/*XY: Non Bracketed*/0.005324,0.013416,/*Bracketed WZ:*/0.009311,0.075276),
        vec4(/*XY: Non Bracketed*/0.013753,0.016519,/*Bracketed WZ:*/0.024376,0.092685),
        vec4(/*XY: Non Bracketed*/0.024700,0.017215,/*Bracketed WZ:*/0.043940,0.096591),
        vec4(/*XY: Non Bracketed*/0.036693,0.015064,/*Bracketed WZ:*/0.065375,0.084521),
        vec4(/*XY: Non Bracketed*/0.047976,0.010684,/*Bracketed WZ:*/0.085539,0.059948),
        vec4(/*XY: Non Bracketed*/0.057015,0.005570,/*Bracketed WZ:*/0.101695,0.031254),
        vec4(/*XY: Non Bracketed*/0.062782,0.001529,/*Bracketed WZ:*/0.112002,0.008578),
        vec4(/*XY: Non Bracketed*/0.064754,0.000000,/*Bracketed WZ:*/0.115526,0.000000),
        vec4(/*XY: Non Bracketed*/0.062782,0.001529,/*Bracketed WZ:*/0.112002,0.008578),
        vec4(/*XY: Non Bracketed*/0.057015,0.005570,/*Bracketed WZ:*/0.101695,0.031254),
        vec4(/*XY: Non Bracketed*/0.047976,0.010684,/*Bracketed WZ:*/0.085539,0.059948),
        vec4(/*XY: Non Bracketed*/0.036693,0.015064,/*Bracketed WZ:*/0.065375,0.084521),
        vec4(/*XY: Non Bracketed*/0.024700,0.017215,/*Bracketed WZ:*/0.043940,0.096591),
        vec4(/*XY: Non Bracketed*/0.013753,0.016519,/*Bracketed WZ:*/0.024376,0.092685),
        vec4(/*XY: Non Bracketed*/0.005324,0.013416,/*Bracketed WZ:*/0.009311,0.075276),
        vec4(/*XY: Non Bracketed*/0.000115,0.009116,/*Bracketed WZ:*/0.000000,0.051147)
);

// 1-Component Filter

const vec4 Kernel0BracketsRealXY_ImZW_1 = vec4(-0.001442,0.672786,0.000000,0.311371);
const vec2 Kernel0Weights_RealX_ImY_1 = vec2(0.767583,1.862321);
const vec4 Kernel0_RealX_ImY_RealZ_ImW_1[] = vec4[](
        vec4(/*XY: Non Bracketed*/-0.001442,0.026656,/*Bracketed WZ:*/0.000000,0.085609),
        vec4(/*XY: Non Bracketed*/0.010488,0.030945,/*Bracketed WZ:*/0.017733,0.099384),
        vec4(/*XY: Non Bracketed*/0.023771,0.030830,/*Bracketed WZ:*/0.037475,0.099012),
        vec4(/*XY: Non Bracketed*/0.036356,0.026770,/*Bracketed WZ:*/0.056181,0.085976),
        vec4(/*XY: Non Bracketed*/0.046822,0.020140,/*Bracketed WZ:*/0.071737,0.064680),
        vec4(/*XY: Non Bracketed*/0.054555,0.012687,/*Bracketed WZ:*/0.083231,0.040745),
        vec4(/*XY: Non Bracketed*/0.059606,0.006074,/*Bracketed WZ:*/0.090738,0.019507),
        vec4(/*XY: Non Bracketed*/0.062366,0.001584,/*Bracketed WZ:*/0.094841,0.005086),
        vec4(/*XY: Non Bracketed*/0.063232,0.000000,/*Bracketed WZ:*/0.096128,0.000000),
        vec4(/*XY: Non Bracketed*/0.062366,0.001584,/*Bracketed WZ:*/0.094841,0.005086),
        vec4(/*XY: Non Bracketed*/0.059606,0.006074,/*Bracketed WZ:*/0.090738,0.019507),
        vec4(/*XY: Non Bracketed*/0.054555,0.012687,/*Bracketed WZ:*/0.083231,0.040745),
        vec4(/*XY: Non Bracketed*/0.046822,0.020140,/*Bracketed WZ:*/0.071737,0.064680),
        vec4(/*XY: Non Bracketed*/0.036356,0.026770,/*Bracketed WZ:*/0.056181,0.085976),
        vec4(/*XY: Non Bracketed*/0.023771,0.030830,/*Bracketed WZ:*/0.037475,0.099012),
        vec4(/*XY: Non Bracketed*/0.010488,0.030945,/*Bracketed WZ:*/0.017733,0.099384),
        vec4(/*XY: Non Bracketed*/-0.001442,0.026656,/*Bracketed WZ:*/0.000000,0.085609)
);
layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4	TextureFarR; 
layout(location = 1) out vec4	TextureFarG; 
layout(location = 2) out vec4	TextureFarB; 
layout(location = 3) out vec2	TextureNearR; 
layout(location = 4) out vec2	TextureNearG; 
layout(location = 5) out vec2	TextureNearB; 
layout(location = 6) out float	TextureWeights; 

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform UniformDOF
{
    float maxRadius;
    float blend;
};

layout(binding = 0) uniform sampler samplerLinear;
layout(binding = 1) uniform sampler samplerPoint;

layout(UPDATE_FREQ_PER_FRAME, binding = 1) uniform texture2D TextureColor;
layout(UPDATE_FREQ_PER_FRAME, binding = 2) uniform texture2D TextureCoC;
layout(UPDATE_FREQ_PER_FRAME, binding = 3) uniform texture2D TextureNearCoC;
layout(UPDATE_FREQ_PER_FRAME, binding = 4) uniform texture2D TextureColorMulFar;

void main()
{
	vec2 step = 1.0f / textureSize(sampler2D(TextureColor, samplerPoint), 0);

    float cocValueFar = texture(sampler2D(TextureCoC, samplerPoint), inUV).g;
    float cocValueNear = texture(sampler2D(TextureNearCoC, samplerPoint), inUV).r;

    TextureFarR = vec4(0, 0, 0, 0);
    TextureFarG = vec4(0, 0, 0, 0);
    TextureFarB = vec4(0, 0, 0, 0);
    TextureNearR = vec2(0, 0);
    TextureNearG = vec2(0, 0);
    TextureNearB = vec2(0, 0);
	TextureWeights = 0;
    
	if(cocValueFar > 0)
	{	
		vec4 valR = vec4(0, 0, 0, 0);
		vec4 valG = vec4(0, 0, 0, 0);
		vec4 valB = vec4(0, 0, 0, 0);

		float total = 0;

		for(int i = 0; i <= KERNEL_RADIUS * 2; i++)
		{
			int index = i - KERNEL_RADIUS;
			vec2 coords = inUV + step * vec2(float(index), 0) * maxRadius;
			
			vec2 c0 = Kernel0_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;
			vec2 c1 = Kernel1_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;
			
			float cocValueSample = texture(sampler2D(TextureCoC, samplerPoint), coords).g;

			if(cocValueSample == 0)
			{
				coords = inUV;
				cocValueSample = cocValueFar;
			}
			
			total += cocValueSample;
			vec3 texel = texture(sampler2D(TextureColorMulFar, samplerLinear), coords).rgb;

			valR += vec4(texel.r * c0, texel.r * c1);
			valG += vec4(texel.g * c0, texel.g * c1);
			valB += vec4(texel.b * c0, texel.b * c1);
		}
		
		TextureWeights = total / (KERNEL_COUNT);
		TextureFarR = valR;
		TextureFarG = valG;
		TextureFarB = valB;
	}

	if(cocValueNear > 0)
	{
		vec2 valR = vec2(0, 0);
		vec2 valG = vec2(0, 0);
		vec2 valB = vec2(0, 0);

		for(int i = 0; i <= KERNEL_RADIUS * 2; i++)
		{
			int index = i - KERNEL_RADIUS;
			vec2 coords = inUV + step * vec2(float(index), 0) * maxRadius;
					
			float cocValueSample = texture(sampler2D(TextureNearCoC, samplerPoint), coords).r;

			if(cocValueSample == 0)
			{
				coords = inUV;
				cocValueSample = cocValueNear;
			}

			vec2 c0 = Kernel0_RealX_ImY_RealZ_ImW_1[index + KERNEL_RADIUS].xy;
			
			vec3 texel = texture(sampler2D(TextureColor, samplerLinear), coords).rgb;

			valR += vec2(texel.r * c0);
			valG += vec2(texel.g * c0);
			valB += vec2(texel.b * c0);
		}
		
		TextureNearR = valR;
		TextureNearG = valG;
		TextureNearB = valB;
	}
}
