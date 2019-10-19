/********************************************************************/
/********************************************************************/
/*         Generated Filter by CircularDofFilterGenerator tool      */
/*     Copyright (c)     Kleber A Garcia  (kecho_garcia@hotmail.com)*/
/*       https://github.com/kecho/CircularDofFilterGenerator        */
/********************************************************************/
/********************************************************************/
/**
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/
static const uint KERNEL_RADIUS = 8;
static const uint KERNEL_COUNT = 17;
static const float4 Kernel0BracketsRealXY_ImZW = float4(-0.038708,0.943062,-0.025574,0.660892);
static const float2 Kernel0Weights_RealX_ImY = float2(0.411259,-0.548794);
static const float4 Kernel0_RealX_ImY_RealZ_ImW[] = {
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
static const float4 Kernel1BracketsRealXY_ImZW = float4(0.000115,0.559524,0.000000,0.178226);
static const float2 Kernel1Weights_RealX_ImY = float2(0.513282,4.561110);
static const float4 Kernel1_RealX_ImY_RealZ_ImW[] = {
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


struct VSOutput 
{
	float4 Position	: SV_POSITION;
    float2 UV		: TEXCOORD0;
};

cbuffer UniformDOF : register(b0, UPDATE_FREQ_PER_FRAME)
{
	float filterRadius;
};

SamplerState	uSampler0		: register(s0);
Texture2D		Texture			: register(t0, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureR		: register(t1, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureG		: register(t2, UPDATE_FREQ_PER_FRAME);
Texture2D		TextureB		: register(t3, UPDATE_FREQ_PER_FRAME);

float2 multComplex(float2 p, float2 q)
{
    return float2(p.x*q.x-p.y*q.y, p.x*q.y+p.y*q.x);
}

float4 main(VSOutput input) : SV_TARGET
{	
	uint w, h;
	TextureR.GetDimensions(w, h);
	float2 step = 1.0f / float2(w, h);
	Texture.GetDimensions(w, h);
	
    float4 valR = float4(0, 0, 0, 0);
    float4 valG = float4(0, 0, 0, 0);
    float4 valB = float4(0, 0, 0, 0);

	for (int i = 0; i <= KERNEL_RADIUS * 2; ++i)
    {
		int index = i - KERNEL_RADIUS;
        float2 coords = input.UV + step * float2(0.0, float(index)) * filterRadius;
        float4 imageTexelR = TextureR.Sample(uSampler0, coords);  
        float4 imageTexelG = TextureG.Sample(uSampler0, coords);  
        float4 imageTexelB = TextureB.Sample(uSampler0, coords);  
        
        float2 c0 = Kernel0_RealX_ImY_RealZ_ImW[index + KERNEL_RADIUS].xy;
        float2 c1 = Kernel1_RealX_ImY_RealZ_ImW[index + KERNEL_RADIUS].xy;

        valR.xy += multComplex(imageTexelR.xy, c0);
        valR.zw += multComplex(imageTexelR.zw, c1);

        valG.xy += multComplex(imageTexelG.xy, c0);
        valG.zw += multComplex(imageTexelG.zw, c1);

        valB.xy += multComplex(imageTexelB.xy, c0); 
        valB.zw += multComplex(imageTexelB.zw, c1);   
    }

    float redChannel   = dot(valR.xy, Kernel0Weights_RealX_ImY) + dot(valR.zw, Kernel1Weights_RealX_ImY);
    float greenChannel = dot(valG.xy, Kernel0Weights_RealX_ImY) + dot(valG.zw, Kernel1Weights_RealX_ImY);
    float blueChannel  = dot(valB.xy, Kernel0Weights_RealX_ImY) + dot(valB.zw, Kernel1Weights_RealX_ImY);
    return float4(sqrt(float3(redChannel, greenChannel, blueChannel)), w);
}