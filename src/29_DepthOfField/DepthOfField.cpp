//EASTL includes
#include "Common_3/ThirdParty/OpenSource/EASTL/vector.h"
#include "Common_3/ThirdParty/OpenSource/EASTL/string.h"

//Interfaces
#include "Common_3/OS/Interfaces/ICameraController.h"
#include "Common_3/OS/Interfaces/ILog.h"
#include "Common_3/OS/Interfaces/IFileSystem.h"
#include "Common_3/OS/Interfaces/ITime.h"
#include "Common_3/OS/Interfaces/IProfiler.h"
#include "Common_3/OS/Interfaces/IInput.h"
#include "Common_3/Renderer/IRenderer.h"
#include "Common_3/OS/Interfaces/IApp.h"
#include "Common_3/Renderer/ResourceLoader.h"

//Math
#include "Common_3/OS/Math/MathTypes.h"

//ui
#include "Middleware_3/UI/AppUI.h"

//input
//asimp importer
#include "Common_3/Tools/AssimpImporter/AssimpImporter.h"
#include "Common_3/OS/Interfaces/IMemory.h"


//--------------------------------------------------------------------------------------------
// GLOBAL DEFINTIONS
//--------------------------------------------------------------------------------------------

uint32_t gFrameIndex = 0;

const uint32_t gImageCount			= 3;

uint32_t gSelectedMethod			= 0;

bool bToggleMicroProfiler			= false;
bool bPrevToggleMicroProfiler		= false;

constexpr float gNear				= 0.1f;
constexpr float gFar				= 300.0f;

static float gFocalPlaneDistance	= 31;
static float gFocalTransitionRange	= 17;

constexpr size_t gPointLights		= 8;
constexpr bool gPauseLights			= false;

//--------------------------------------------------------------------------------------------
// STRUCT DEFINTIONS
//--------------------------------------------------------------------------------------------

struct MeshBatch
{
	eastl::vector<float3> PositionsData;
	eastl::vector<uint>   IndicesData;

	Buffer* pPositionStream;
	Buffer* pNormalStream;
	Buffer* pUVStream;
	Buffer* pIndicesStream;

	uint32_t	mCountVertices;
	uint32_t	mCountIndices;
	int			MaterialID;
};

struct SceneData
{
	mat4						WorldMatrix;
	eastl::vector<MeshBatch*>	MeshBatches;
	Buffer* pConstantBuffer;
};

struct cbPerObj
{
	mat4 mWorldMat;
};

struct cbPerPass
{
	mat4 mProjectMat;
	mat4 mViewMat;
};

struct UniformDataDOF
{
	float filterRadius	= 1.0f;
	float blend			= 10.0f;
	float nb			= gNear;
	float ne			= 0.0f;
	float fb			= 0.0f;
	float fe			= gFar;
	float2 projParams	= {};
};

struct PointLight
{
	alignas(16) float3 position;
	alignas(16) float3 ambient;
	alignas(16) float3 diffuse;
	alignas(16) float3 specular;
	alignas(16) float3 attenuationParams;
};

struct
{
	float3 position[gPointLights];
	float3 color[gPointLights];
} lightInstancedata;

//--------------------------------------------------------------------------------------------
// RENDERING PIPELINE DATA
//--------------------------------------------------------------------------------------------

Renderer* pRenderer																= NULL;

Queue* pGraphicsQueue															= NULL;

CmdPool* pCmdPool																= NULL;
Cmd** ppCmdsHDR 																= NULL;
Cmd** ppCmdsCoC 																= NULL;
Cmd** ppCmdsDownress 															= NULL;
Cmd** ppCmdsMaxFilterNearCoCX 													= NULL;
Cmd** ppCmdsMaxFilterNearCoCY													= NULL;
Cmd** ppCmdsBoxFilterNearCoCX 													= NULL;
Cmd** ppCmdsBoxFilterNearCoCY													= NULL;
Cmd** ppCmdsComputation 														= NULL;
Cmd** ppCmdsFilling																= NULL;
Cmd** ppCmdsComposite															= NULL;

SwapChain* pSwapChain 															= NULL;

Fence* pRenderCompleteFences[gImageCount] 										= { NULL };

Semaphore* pImageAcquiredSemaphore 												= NULL;
Semaphore* pRenderCompleteSemaphores[gImageCount] 								= { NULL };

// Shared: Circular, Gather-Based
Pipeline* pPipelineScene 														= NULL;
Pipeline* pPipelineLight 														= NULL;
Pipeline* pPipelineCoC 															= NULL;
Pipeline* pPipelineMaxFilterNearCoCX 											= NULL;
Pipeline* pPipelineMaxFilterNearCoCY 											= NULL;
Pipeline* pPipelineBoxFilterNearCoCX 											= NULL;
Pipeline* pPipelineBoxFilterNearCoCY 											= NULL;

struct
{
	Pipeline* pPipelineDownres 													= NULL;
	Pipeline* pPipelineHorizontalDOF 											= NULL;
	Pipeline* pPipelineComposite 												= NULL;
} PipelinesCircularDoF;

struct
{
	Pipeline* pPipelineDownres 													= NULL;
	Pipeline* pPipelineComputation 												= NULL;
	Pipeline* pPipelineFilling 													= NULL;
	Pipeline* pPipelineComposite 												= NULL;
} PipelinesGatherBased;

struct
{
	Pipeline* pPipelineDOF 														= NULL;
} PipelinesSinglePass;

// Meshes
Shader* pShaderBasic 															= NULL;
Shader* pShaderLight 															= NULL;

// Shared: Circular, Gather-Based
Shader* pShaderGenCoc 															= NULL;
Shader* pShaderMaxFilterNearCoCX 												= NULL;
Shader* pShaderMaxFilterNearCoCY 												= NULL;
Shader* pShaderBoxFilterNearCoCX 												= NULL;
Shader* pShaderBoxFilterNearCoCY 												= NULL;

struct
{
	Shader* pShaderDownres 														= NULL;
	Shader* pShaderHorizontalDof 												= NULL;
	Shader* pShaderComposite 													= NULL;
}	ShadersCircularDoF;

struct
{
	Shader* pShaderDownres 														= NULL;
	Shader* pShaderComputation													= NULL;
	Shader* pShaderFill														= NULL;
	Shader* pShaderComposite 													= NULL;
}	ShadersGatherBased;

struct
{
	Shader* pShaderDOF 																= NULL;
}	ShadersSinglePass;

DescriptorSet* pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_COUNT] 				= { NULL };
DescriptorSet* pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_COUNT] 				= { NULL };
DescriptorSet* pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };

struct
{
	DescriptorSet* pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_COUNT] 			= { NULL };
	DescriptorSet* pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
	DescriptorSet* pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
}	DescriptorsCircularDoF;

struct
{
	DescriptorSet* pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_COUNT] 			= { NULL };
	DescriptorSet* pDescriptorSetsComputationPass[DESCRIPTOR_UPDATE_FREQ_COUNT] 	= { NULL };
	DescriptorSet* pDescriptorSetsFillingPass[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
	DescriptorSet* pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
}	DescriptorsGatherBased;

struct
{
	DescriptorSet* pDescriptorSetsDOF[DESCRIPTOR_UPDATE_FREQ_COUNT] 				= { NULL };
}	DescriptorsSinglePass;

RootSignature* pRootSignatureScene 												= NULL;
RootSignature* pRootSignatureCoC 												= NULL;
RootSignature* pRootSignatureMaxFilterNearX 									= NULL;
RootSignature* pRootSignatureMaxFilterNearY 									= NULL;
RootSignature* pRootSignatureBoxFilterNearX 									= NULL;
RootSignature* pRootSignatureBoxFilterNearY 									= NULL;

struct
{
	RootSignature* pRootSignatureDownres 											= NULL;
	RootSignature* pRootSignatureHorizontalPass										= NULL;
	RootSignature* pRootSignatureCompositePass 										= NULL;
} RootSignaturesCircularDoF;

struct
{
	RootSignature* pRootSignatureDownres 											= NULL;
	RootSignature* pRootSignatureComputationPass									= NULL;
	RootSignature* pRootSignatureFillingPass 										= NULL;
	RootSignature* pRootSignatureCompositePass 										= NULL;
} RootSignaturesGatherBased;

struct
{
	RootSignature* pRootSignatureDOF 												= NULL;
} RootSignaturesSinglePass;

Sampler* pSamplerLinear;
Sampler* pSamplerLinearClampEdge;
Sampler* pSamplerPoint;
Sampler* pSamplerAnisotropic;

RenderTarget* pDepthBuffer;

// Shared: Circular, Gather-Based
RenderTarget* pRenderTargetHDR[gImageCount]										= { NULL }; // R16G16B16A16
RenderTarget* pRenderTargetCoC[gImageCount]										= { NULL }; // R8G8

RenderTarget* pRenderTargetCoCDowres[gImageCount]								= { NULL }; // COC/4
RenderTarget* pRenderTargetColorDownres[gImageCount]							= { NULL };	// HDR/4
RenderTarget* pRenderTargetMulFarDownres[gImageCount]							= { NULL };	// HDR/4

RenderTarget* pRenderTargetFilterNearCoC[gImageCount]							= { NULL };	// R8/4
RenderTarget* pRenderTargetFilterNearCoCFinal[gImageCount]						= { NULL };	// R8/4

struct
{
	RenderTarget* pRenderTargetFarR[gImageCount]									= { NULL }; // FAR/4
	RenderTarget* pRenderTargetFarG[gImageCount]									= { NULL }; // FAR/4
	RenderTarget* pRenderTargetFarB[gImageCount]									= { NULL }; // FAR/4

	RenderTarget* pRenderTargetNearR[gImageCount]									= { NULL }; // NEAR/4
	RenderTarget* pRenderTargetNearG[gImageCount]									= { NULL }; // NEAR/4
	RenderTarget* pRenderTargetNearB[gImageCount]									= { NULL }; // NEAR/4
	RenderTarget* pRenderTargetWeights[gImageCount]									= { NULL };
} RenderTargetsCircularDoF;

struct
{
	RenderTarget* pRenderTargetFar[gImageCount]										= { NULL }; // FAR/4
	RenderTarget* pRenderTargetNear[gImageCount]									= { NULL }; // NEAR/4

	RenderTarget* pRenderTargetFarFilled[gImageCount]								= { NULL }; // FAR/4
	RenderTarget* pRenderTargetNearFilled[gImageCount]								= { NULL }; // FAR/4
} RenderTargetsGatherBased;

RasterizerState* pRasterDefault 												= NULL;

DepthState* pDepthDefault 														= NULL;
DepthState* pDepthNone 															= NULL;

Buffer* pUniformBuffersProjView[gImageCount] 									= { NULL };
Buffer* pUniformBuffersDOF[gImageCount] 										= { NULL };

Buffer* pPointLightsBuffer														= NULL;
Buffer* pInstancePositionBuffer													= NULL;
Buffer* pInstanceColorBuffer													= NULL;

cbPerPass			gUniformDataScene;
UniformDataDOF		gUniformDataDOF;
SceneData			gSponzaSceneData;

MeshBatch* gLightMesh;

PointLight			pointLights[gPointLights];

//--------------------------------------------------------------------------------------------
// Sponza Data
//--------------------------------------------------------------------------------------------

#define TOTAL_SPONZA_IMGS 81

Texture* pMaterialTexturesSponza[TOTAL_SPONZA_IMGS]							= { NULL };

eastl::vector<int>	gSponzaTextureIndexforMaterial;

const char* gModel_Sponza_File = "sponza.obj";
const char* gModel_Light_File = "sphere.obj";

#if defined(TARGET_IOS) || defined(__ANDROID__)
const char* pTextureName [] = { "albedoMap" };
#endif

const char* pMaterialImageFileNames [] = {
	"SponzaPBR_Textures/ao",
	"SponzaPBR_Textures/ao",
	"SponzaPBR_Textures/ao",
	"SponzaPBR_Textures/ao",
	"SponzaPBR_Textures/ao",

	//common
	"SponzaPBR_Textures/ao",
	"SponzaPBR_Textures/Dielectric_metallic",
	"SponzaPBR_Textures/Metallic_metallic",
	"SponzaPBR_Textures/gi_flag",

	//Background - 9
	"SponzaPBR_Textures/Background/Background_Albedo",
	"SponzaPBR_Textures/Background/Background_Normal",
	"SponzaPBR_Textures/Background/Background_Roughness",

	//ChainTexture - 12
	"SponzaPBR_Textures/ChainTexture/ChainTexture_Albedo",
	"SponzaPBR_Textures/ChainTexture/ChainTexture_Metallic",
	"SponzaPBR_Textures/ChainTexture/ChainTexture_Normal",
	"SponzaPBR_Textures/ChainTexture/ChainTexture_Roughness",

	//Lion - 16
	"SponzaPBR_Textures/Lion/Lion_Albedo",
	"SponzaPBR_Textures/Lion/Lion_Normal",
	"SponzaPBR_Textures/Lion/Lion_Roughness",

	//Sponza_Arch - 19
	"SponzaPBR_Textures/Sponza_Arch/Sponza_Arch_diffuse",
	"SponzaPBR_Textures/Sponza_Arch/Sponza_Arch_normal",
	"SponzaPBR_Textures/Sponza_Arch/Sponza_Arch_roughness",

	//Sponza_Bricks - 22
	"SponzaPBR_Textures/Sponza_Bricks/Sponza_Bricks_a_Albedo",
	"SponzaPBR_Textures/Sponza_Bricks/Sponza_Bricks_a_Normal",
	"SponzaPBR_Textures/Sponza_Bricks/Sponza_Bricks_a_Roughness",

	//Sponza_Ceiling - 25
	"SponzaPBR_Textures/Sponza_Ceiling/Sponza_Ceiling_diffuse",
	"SponzaPBR_Textures/Sponza_Ceiling/Sponza_Ceiling_normal",
	"SponzaPBR_Textures/Sponza_Ceiling/Sponza_Ceiling_roughness",

	//Sponza_Column - 28
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_a_diffuse",
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_a_normal",
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_a_roughness",

	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_b_diffuse",
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_b_normal",
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_b_roughness",

	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_c_diffuse",
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_c_normal",
	"SponzaPBR_Textures/Sponza_Column/Sponza_Column_c_roughness",

	//Sponza_Curtain - 46
	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_Blue_diffuse",
	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_Blue_normal",

	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_Green_diffuse",
	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_Green_normal",

	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_Red_diffuse",
	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_Red_normal",

	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_metallic",
	"SponzaPBR_Textures/Sponza_Curtain/Sponza_Curtain_roughness",

	//Sponza_Details - 54
	"SponzaPBR_Textures/Sponza_Details/Sponza_Details_diffuse",
	"SponzaPBR_Textures/Sponza_Details/Sponza_Details_metallic",
	"SponzaPBR_Textures/Sponza_Details/Sponza_Details_normal",
	"SponzaPBR_Textures/Sponza_Details/Sponza_Details_roughness",

	//Sponza_Fabric - 58
	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_Blue_diffuse",
	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_Blue_normal",

	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_Green_diffuse",
	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_Green_normal",

	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_metallic",
	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_roughness",

	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_Red_diffuse",
	"SponzaPBR_Textures/Sponza_Fabric/Sponza_Fabric_Red_normal",

	//Sponza_FlagPole - 66
	"SponzaPBR_Textures/Sponza_FlagPole/Sponza_FlagPole_diffuse",
	"SponzaPBR_Textures/Sponza_FlagPole/Sponza_FlagPole_normal",
	"SponzaPBR_Textures/Sponza_FlagPole/Sponza_FlagPole_roughness",

	//Sponza_Floor - 69
	"SponzaPBR_Textures/Sponza_Floor/Sponza_Floor_diffuse",
	"SponzaPBR_Textures/Sponza_Floor/Sponza_Floor_normal",
	"SponzaPBR_Textures/Sponza_Floor/Sponza_Floor_roughness",

	//Sponza_Roof - 72
	"SponzaPBR_Textures/Sponza_Roof/Sponza_Roof_diffuse",
	"SponzaPBR_Textures/Sponza_Roof/Sponza_Roof_normal",
	"SponzaPBR_Textures/Sponza_Roof/Sponza_Roof_roughness",

	//Sponza_Thorn - 75
	"SponzaPBR_Textures/Sponza_Thorn/Sponza_Thorn_diffuse",
	"SponzaPBR_Textures/Sponza_Thorn/Sponza_Thorn_normal",
	"SponzaPBR_Textures/Sponza_Thorn/Sponza_Thorn_roughness",

	//Vase - 78
	"SponzaPBR_Textures/Vase/Vase_diffuse",
	"SponzaPBR_Textures/Vase/Vase_normal",
	"SponzaPBR_Textures/Vase/Vase_roughness",

	//VaseHanging - 81
	"SponzaPBR_Textures/VaseHanging/VaseHanging_diffuse",
	"SponzaPBR_Textures/VaseHanging/VaseHanging_normal",
	"SponzaPBR_Textures/VaseHanging/VaseHanging_roughness",

	//VasePlant - 84
	"SponzaPBR_Textures/VasePlant/VasePlant_diffuse",
	"SponzaPBR_Textures/VasePlant/VasePlant_normal",
	"SponzaPBR_Textures/VasePlant/VasePlant_roughness",

	//VaseRound - 87
	"SponzaPBR_Textures/VaseRound/VaseRound_diffuse",
	"SponzaPBR_Textures/VaseRound/VaseRound_normal",
	"SponzaPBR_Textures/VaseRound/VaseRound_roughness",

	/*
// 90
"lion/lion_albedo",
"lion/lion_specular",
"lion/lion_normal",
	*/
};

//--------------------------------------------------------------------------------------------
// THE FORGE OBJECTS
//--------------------------------------------------------------------------------------------

UIApp gAppUI;
GuiComponent* pGui								= NULL;
VirtualJoystickUI gVirtualJoystick;
GpuProfiler* pGpuProfiler						= NULL;
ICameraController* pCameraController			= NULL;
TextDrawDesc gFrameTimeDraw						= TextDrawDesc(0, 0xff00f0ff, 18);

class DepthOfField: public IApp
{
	public:
	DepthOfField()
	{
		mSettings.mWidth = 1280;
		mSettings.mHeight = 720;

		#if defined(TARGET_IOS)
		mSettings.mContentScaleFactor = 1.f;
		#endif
	}

	bool Init()
	{
		// FILE PATHS
		PathHandle programDirectory = fsCopyProgramDirectoryPath();
		FileSystem* fileSystem = fsGetPathFileSystem(programDirectory);
		if (!fsPlatformUsesBundledResources())
		{
			PathHandle resourceDirRoot = fsAppendPathComponent(programDirectory, "../../../../src/29_DepthOfField");
			fsSetResourceDirectoryRootPath(resourceDirRoot);

			fsSetRelativePathForResourceDirectory(RD_TEXTURES, "../../../The-Forge/Art/Sponza/Textures");
			fsSetRelativePathForResourceDirectory(RD_MESHES, "../../../The-Forge/Examples_3/Unit_Tests/UnitTestResources/Meshes");
			fsSetRelativePathForResourceDirectory(RD_BUILTIN_FONTS, "../../../The-Forge/Examples_3/Unit_Tests/UnitTestResources/Fonts");
			fsSetRelativePathForResourceDirectory(RD_ANIMATIONS, "../../../The-Forge/Examples_3/Unit_Tests/UnitTestResources/Animation");
			fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_TEXT, "../../../The-Forge/Middleware_3/Text");
			fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_UI, "../../../The-Forge/Middleware_3/UI");
		}

		// window and render setup
		RendererDesc settings = { 0 };
		initRenderer(GetName(), &settings, &pRenderer);
		// check for init success
		if (!pRenderer) return false;

		QueueDesc queueDesc = {};
		queueDesc.mType = CMD_POOL_DIRECT;
		queueDesc.mFlag = QUEUE_FLAG_NONE;
		addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

		AddCmds();

		AddSyncObjects();

		// Resource Loading
		initResourceLoaderInterface(pRenderer);

		AddShaders();

		AddPipelineStateObjects();

		AddSamplers();

		AddDescriptorSets();

		AddBuffers();

		bool succeed = LoadSponza();

		if (!succeed)
		{
			finishResourceLoading();
			return false;
		}

		finishResourceLoading();

		//--------------------------------------------------------------------------------------------
		// UI, Camera, Input
		//--------------------------------------------------------------------------------------------

		if (!gAppUI.Init(pRenderer))
			return false;

		gAppUI.LoadFont("TitilliumText/TitilliumText-Bold.otf", RD_BUILTIN_FONTS);

		#if defined(__ANDROID__) || defined(TARGET_IOS)
		if (!gVirtualJoystick.Init(pRenderer, "circlepad", RD_TEXTURES))
			return false;
		#endif

		GuiDesc guiDesc = {};
		float   dpiScale = getDpiScale().x;
		guiDesc.mStartSize = vec2(140.0f, 320.0f);
		guiDesc.mStartPosition = vec2(mSettings.mWidth / dpiScale - guiDesc.mStartSize.getX() * 1.1f, guiDesc.mStartSize.getY() * 0.5f);

		pGui = gAppUI.AddGuiComponent("Micro profiler", &guiDesc);

		const char* method_names[3] =
		{
			"Circular DOF",
			"Practical Gather Based",
			"Single Pass"
		};

		const uint32_t method_indexes[3] = { 0, 1 , 2 };

		pGui->AddWidget(CheckboxWidget("Toggle Micro Profiler", &bToggleMicroProfiler));
		pGui->AddWidget(DropdownWidget("Method", &gSelectedMethod, method_names, method_indexes, 3));
		pGui->AddWidget(SliderFloatWidget("Focal Plane Distance", &gFocalPlaneDistance, gNear, gFar, 1.0f, "%.1f"));
		pGui->AddWidget(SliderFloatWidget("Focal Transition Range", &gFocalTransitionRange, 0, 1000, 1.0f, "%.1f"));
		pGui->AddWidget(SliderFloatWidget("Blend", &gUniformDataDOF.blend, 0, 10, 0.11f, "%.1f"));
		pGui->AddWidget(SliderFloatWidget("Max Radius", &gUniformDataDOF.filterRadius, 0, 10, 0.1f, "%.1f"));

		CameraMotionParameters cmp { 100.0f, 100.0f, 500.0f };
		vec3                   camPos { 100.0f, 25.0f, 0.0f };
		vec3                   lookAt { 0 };

		pCameraController = createFpsCameraController(camPos, lookAt);

		pCameraController->setMotionParameters(cmp);

		if (!initInputSystem(pWindow))
			return false;

		// Initialize microprofiler and it's UI.
		initProfiler();

		// Gpu profiler can only be added after initProfile.
		addGpuProfiler(pRenderer, pGraphicsQueue, &pGpuProfiler, "GpuProfiler");

		// App Actions
		InputActionDesc actionDesc = { InputBindings::BUTTON_FULLSCREEN, [](InputActionContext* ctx) { toggleFullscreen(((IApp*)ctx->pUserData)->pWindow); return true; }, this };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::BUTTON_EXIT, [](InputActionContext* ctx) { requestShutdown(); return true; } };
		addInputAction(&actionDesc);
		actionDesc =
		{
			InputBindings::BUTTON_ANY, [](InputActionContext* ctx)
			{
				bool capture = gAppUI.OnButton(ctx->mBinding, ctx->mBool, ctx->pPosition);
				setEnableCaptureInput(capture && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
				return true;
			}, this
		};
		addInputAction(&actionDesc);
		typedef bool (*CameraInputHandler)(InputActionContext* ctx, uint32_t index);
		static CameraInputHandler onCameraInput = [](InputActionContext* ctx, uint32_t index)
		{
			if (!bToggleMicroProfiler && !gAppUI.IsFocused() && *ctx->pCaptured)
			{
				#if defined(TARGET_IOS) || defined(__ANDROID__)
				gVirtualJoystick.OnMove(index, ctx->mPhase != INPUT_ACTION_PHASE_CANCELED, ctx->pPosition);
				#endif				
				index ? pCameraController->onRotate(ctx->mFloat2) : pCameraController->onMove(ctx->mFloat2);
			}
			return true;
		};
		actionDesc = { InputBindings::FLOAT_RIGHTSTICK, [](InputActionContext* ctx) { return onCameraInput(ctx, 1); }, NULL, 20.0f, 200.0f, 0.5f };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::FLOAT_LEFTSTICK, [](InputActionContext* ctx) { return onCameraInput(ctx, 0); }, NULL, 20.0f, 200.0f, 1.0f };
		addInputAction(&actionDesc);
		actionDesc = { InputBindings::BUTTON_NORTH, [](InputActionContext* ctx) { pCameraController->resetView(); return true; } };
		addInputAction(&actionDesc);

		return true;
	}

	void Exit()
	{
		waitQueueIdle(pGraphicsQueue);

		exitInputSystem();

		destroyCameraController(pCameraController);

		#if defined(TARGET_IOS) || defined(__ANDROID__)
		gVirtualJoystick.Exit();
		#endif

		gAppUI.Exit();

		exitProfiler();

		RemoveSyncObjects();

		UnloadSponza();

		RemoveBuffers();

		RemoveDescriptorSets();

		RemoveSamplers();

		RemoveShaders();

		RemovePipelineStateObjects();

		RemoveCmds();

		removeGpuProfiler(pRenderer, pGpuProfiler);
		removeResourceLoaderInterface(pRenderer);
		removeQueue(pGraphicsQueue);
		removeRenderer(pRenderer);
	}

	bool Load()
	{
		if (!addSwapChain()) return false;

		AddRenderTargets();

		if (!gAppUI.Load(pSwapChain->ppSwapchainRenderTargets))
			return false;


		#if defined(TARGET_IOS) || defined(__ANDROID__)
		if (!gVirtualJoystick.Load(pSwapChain->ppSwapchainRenderTargets[0]))
			return false;
		#endif

		loadProfiler(&gAppUI, mSettings.mWidth, mSettings.mHeight);

		AddPipelines();

		PrepareDescriptorSets();

		pCameraController->moveTo(vec3(-104, 96, 3.5));
		pCameraController->lookAt(vec3(5, 92, 0));

		for (int i = 0; i < gPointLights; ++i)
		{
			pointLights[i].attenuationParams = float3 { 1.0f,  0.045f,  0.0075f };
			pointLights[i].ambient = float3 { 0.1f, 0.1f, 0.1f };
			pointLights[i].position = float3 { -62.0f + 37.0f * (i / 2), 87.5f,  (i % 2) ? 8.5f : -1.5f };

			pointLights[i].diffuse = float3 {
				2.0f * (rand() % 255) / 255.0f,
				2.0f * (rand() % 255) / 255.0f,
				2.0f * (rand() % 255) / 255.0f };

			pointLights[i].specular = pointLights[i].diffuse;
		}

		return true;
	}

	void Unload()
	{
		waitQueueIdle(pGraphicsQueue);

		unloadProfiler();
		gAppUI.Unload();


		#if defined(TARGET_IOS) || defined(__ANDROID__)
		gVirtualJoystick.Unload();
		#endif

		RemovePipelines();
		RemoveRenderTargets();

		removeSwapChain(pRenderer, pSwapChain);
		pDepthBuffer = NULL;
		pSwapChain = NULL;
	}

	void Update(float deltaTime)
	{
		updateInputSystem(mSettings.mWidth, mSettings.mHeight);
		pCameraController->update(deltaTime);

		static float currentTime;
		currentTime += deltaTime;

		static float currentLightTime;
		if (!gPauseLights)
			currentLightTime += deltaTime;

		// update camera with time
		mat4 viewMat = pCameraController->getViewMatrix();

		const float aspectInverse =
			(float)mSettings.mHeight / (float)mSettings.mWidth;
		const float horizontal_fov = PI / 2.0f;
		mat4 projMat =
			mat4::perspective(horizontal_fov, aspectInverse, gNear, gFar);

		gUniformDataScene.mProjectMat = projMat;
		gUniformDataScene.mViewMat =  viewMat;

		viewMat.setTranslation(vec3(0));

		gUniformDataDOF.nb = gNear;
		gUniformDataDOF.ne = gFocalPlaneDistance - gFocalTransitionRange;
		if (gUniformDataDOF.ne < gNear)
			gUniformDataDOF.ne = gNear;
		gUniformDataDOF.fb = gFocalPlaneDistance + gFocalTransitionRange;
		gUniformDataDOF.fe= gFar;

		gUniformDataDOF.projParams = { projMat[2][2], projMat[3][2] };

		for (int i = 0; i < gPointLights; ++i)
		{
			lightInstancedata.position[i] = pointLights[i].position;
			lightInstancedata.color[i] = pointLights[i].diffuse;
		}

		if (bToggleMicroProfiler != bPrevToggleMicroProfiler)
		{
			toggleProfiler();
			bPrevToggleMicroProfiler = bToggleMicroProfiler;
		}

		/************************************************************************/
		// Update GUI
		/************************************************************************/
		gAppUI.Update(deltaTime);
	}

	void RecordCommandBuffersCircularDoF(eastl::vector<Cmd*>& allCmds)
	{
		RenderTarget* pSwapChainRenderTarget = pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

		RenderTarget* pHdrRenderTarget = pRenderTargetHDR[gFrameIndex];
		RenderTarget* pGenCocRenderTarget = pRenderTargetCoC[gFrameIndex];

		RenderTarget* pDownresRenderTargets[3] =
		{
			pRenderTargetCoCDowres[gFrameIndex],
			pRenderTargetColorDownres[gFrameIndex],
			pRenderTargetMulFarDownres[gFrameIndex],
		};

		RenderTarget* pFilteredNearCoC = pRenderTargetFilterNearCoC[gFrameIndex];
		RenderTarget* pFilteredNearCoCFinal = pRenderTargetFilterNearCoCFinal[gFrameIndex];

		RenderTarget* pHorizontalRenderTargets[7] =
		{
			RenderTargetsCircularDoF.pRenderTargetFarR[gFrameIndex],
			RenderTargetsCircularDoF.pRenderTargetFarG[gFrameIndex],
			RenderTargetsCircularDoF.pRenderTargetFarB[gFrameIndex],
			RenderTargetsCircularDoF.pRenderTargetNearR[gFrameIndex],
			RenderTargetsCircularDoF.pRenderTargetNearG[gFrameIndex],
			RenderTargetsCircularDoF.pRenderTargetNearB[gFrameIndex],
			RenderTargetsCircularDoF.pRenderTargetWeights[gFrameIndex],
		};

		// Load Actions
		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.depth = 1.0f;
		loadActions.mLoadActionStencil = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.stencil = 0;

		Cmd* cmd = ppCmdsDownress[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Downres Pass", true);

			TextureBarrier textureBarriers[5] =
			{
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[2]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 5, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 3; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 3, pDownresRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pDownresRenderTargets[0]->mDesc.mWidth,
				(float)pDownresRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pDownresRenderTargets[0]->mDesc.mWidth,
				pDownresRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesCircularDoF.pPipelineDownres);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsCircularDoF.pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsCircularDoF.pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsMaxFilterNearCoCX[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Max X", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);
			
			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoC, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoC->mDesc.mWidth,
				(float)pFilteredNearCoC->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoC->mDesc.mWidth,
				pFilteredNearCoC->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineMaxFilterNearCoCX);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsMaxFilterNearCoCY[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Max Y", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoCFinal, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoCFinal->mDesc.mWidth,
				(float)pFilteredNearCoCFinal->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoCFinal->mDesc.mWidth,
				pFilteredNearCoCFinal->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineMaxFilterNearCoCY);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsBoxFilterNearCoCX[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Blur X", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoC, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoC->mDesc.mWidth,
				(float)pFilteredNearCoC->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoC->mDesc.mWidth,
				pFilteredNearCoC->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineBoxFilterNearCoCX);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsBoxFilterNearCoCY[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Blur Y", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoCFinal, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoCFinal->mDesc.mWidth,
				(float)pFilteredNearCoCFinal->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoCFinal->mDesc.mWidth,
				pFilteredNearCoCFinal->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineBoxFilterNearCoCY);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsComputation[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Horizontal Pass", true);

			TextureBarrier textureBarriers[11] =
			{
				{ pHorizontalRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[2]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[3]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[4]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[5]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[6]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },	// NearCoC DownRes
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // CoC DownRes
				{ pDownresRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // Color DownRes
				{ pDownresRenderTargets[2]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },	// Color DownRes * FarCoC
			};

			cmdResourceBarrier(cmd, 0, nullptr, 11, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 7; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}

			cmdBindRenderTargets(cmd, 7, pHorizontalRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pHorizontalRenderTargets[0]->mDesc.mWidth,
				(float)pHorizontalRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pHorizontalRenderTargets[0]->mDesc.mWidth,
				pHorizontalRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesCircularDoF.pPipelineHorizontalDOF);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsComposite[gFrameIndex];
		beginCmd(cmd);
		{
			RenderTarget* pSwapChainRenderTarget =
				pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Vertical + Composite Pass", true);

			TextureBarrier textureBarriers[11] =
			{
				{ pSwapChainRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[2]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[3]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[4]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[5]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[6]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 11, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pSwapChainRenderTarget->mDesc.mWidth,
				(float)pSwapChainRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pSwapChainRenderTarget->mDesc.mWidth,
				pSwapChainRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesCircularDoF.pPipelineComposite);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsCircularDoF.pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsCircularDoF.pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

			loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_LOAD;
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Draw UI", true);
			static HiresTimer gTimer;
			gTimer.GetUSec(true);

			#if defined(TARGET_IOS) || defined(__ANDROID__)
			gVirtualJoystick.Draw(cmd, { 1.0f, 1.0f, 1.0f, 1.0f });
			#endif

			gAppUI.DrawText(cmd, float2(8, 15), eastl::string().sprintf("CPU %f ms", gTimer.GetUSecAverage() / 1000.0f).c_str(), &gFrameTimeDraw);

			#if !defined(__ANDROID__)
			gAppUI.DrawText(
				cmd, float2(8, 40), eastl::string().sprintf("GPU %f ms", (float)pGpuProfiler->mCumulativeTime * 1000.0f).c_str(),
				&gFrameTimeDraw);
			gAppUI.DrawDebugGpuProfile(cmd, float2(8, 65), pGpuProfiler, NULL);
			#endif

			cmdDrawProfiler();

			gAppUI.Gui(pGui);

			gAppUI.Draw(cmd);
			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

			textureBarriers[0] = { pSwapChainRenderTarget->pTexture, RESOURCE_STATE_PRESENT };
			cmdResourceBarrier(cmd, 0, NULL, 1, textureBarriers);
		}
		cmdEndGpuFrameProfile(cmd, pGpuProfiler);
		endCmd(cmd);
		allCmds.push_back(cmd);
	}

	void RecordCommandBuffersGatherBased(eastl::vector<Cmd*>& allCmds)
	{
		// Load Actions
		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.depth = 1.0f;
		loadActions.mLoadActionStencil = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.stencil = 0;

		RenderTarget* pSwapChainRenderTarget = pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

		RenderTarget* pHdrRenderTarget = pRenderTargetHDR[gFrameIndex];
		RenderTarget* pGenCocRenderTarget = pRenderTargetCoC[gFrameIndex];

		RenderTarget* pDownresRenderTargets[3] =
		{
			pRenderTargetCoCDowres[gFrameIndex],
			pRenderTargetColorDownres[gFrameIndex],
			pRenderTargetMulFarDownres[gFrameIndex]
		};

		RenderTarget* pFilteredNearCoC = pRenderTargetFilterNearCoC[gFrameIndex];
		RenderTarget* pFilteredNearCoCFinal = pRenderTargetFilterNearCoCFinal[gFrameIndex];

		RenderTarget* pComputationRenderTargets[2] =
		{
			RenderTargetsGatherBased.pRenderTargetNear[gFrameIndex],
			RenderTargetsGatherBased.pRenderTargetFar[gFrameIndex]
		};

		RenderTarget* pFillingRenderTargets[2] =
		{
			RenderTargetsGatherBased.pRenderTargetNearFilled[gFrameIndex],
			RenderTargetsGatherBased.pRenderTargetFarFilled[gFrameIndex]
		};

		Cmd* cmd = ppCmdsDownress[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Downres Pass", true);

			TextureBarrier textureBarriers[5] =
			{
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[2]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 5, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 3; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 3, pDownresRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pDownresRenderTargets[0]->mDesc.mWidth,
				(float)pDownresRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pDownresRenderTargets[0]->mDesc.mWidth,
				pDownresRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesGatherBased.pPipelineDownres);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsGatherBased.pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsGatherBased.pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsMaxFilterNearCoCX[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Max X", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoC, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoC->mDesc.mWidth,
				(float)pFilteredNearCoC->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoC->mDesc.mWidth,
				pFilteredNearCoC->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineMaxFilterNearCoCX);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsMaxFilterNearCoCY[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Max Y", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoCFinal, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoCFinal->mDesc.mWidth,
				(float)pFilteredNearCoCFinal->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoCFinal->mDesc.mWidth,
				pFilteredNearCoCFinal->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineMaxFilterNearCoCY);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsBoxFilterNearCoCX[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Blur X", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoC, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoC->mDesc.mWidth,
				(float)pFilteredNearCoC->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoC->mDesc.mWidth,
				pFilteredNearCoC->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineBoxFilterNearCoCX);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsBoxFilterNearCoCY[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "NearCoC Blur Y", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoC->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoCFinal, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoCFinal->mDesc.mWidth,
				(float)pFilteredNearCoCFinal->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoCFinal->mDesc.mWidth,
				pFilteredNearCoCFinal->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineBoxFilterNearCoCY);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsComputation[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Computation Pass", true);

			TextureBarrier textureBarriers[6] =
			{
				{ pComputationRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pComputationRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // CoC DownRes
				{ pDownresRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // Color DownRes
				{ pDownresRenderTargets[2]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // CoC Mul Far
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },	// Near CoC Blurred
			};

			cmdResourceBarrier(cmd, 0, nullptr, 6, textureBarriers);

			loadActions = {};
			for(int i = 0; i < 2; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}

			cmdBindRenderTargets(cmd, 2, pComputationRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pComputationRenderTargets[0]->mDesc.mWidth,
				(float)pComputationRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pComputationRenderTargets[0]->mDesc.mWidth,
				pComputationRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesGatherBased.pPipelineComputation);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsGatherBased.pDescriptorSetsComputationPass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsGatherBased.pDescriptorSetsComputationPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsFilling[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Filling Pass", true);

			TextureBarrier textureBarriers[6] =
			{
				{ pFillingRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFillingRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pComputationRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pComputationRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
			};

			cmdResourceBarrier(cmd, 0, nullptr, 6, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 2; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}

			cmdBindRenderTargets(cmd, 2, pFillingRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFillingRenderTargets[0]->mDesc.mWidth,
				(float)pFillingRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFillingRenderTargets[0]->mDesc.mWidth,
				pFillingRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesGatherBased.pPipelineFilling);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsGatherBased.pDescriptorSetsFillingPass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsGatherBased.pDescriptorSetsFillingPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsComposite[gFrameIndex];
		beginCmd(cmd);
		{
			RenderTarget* pSwapChainRenderTarget =
				pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Composite Pass", true);

			TextureBarrier textureBarriers[7] =
			{
				{ pSwapChainRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFillingRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pFillingRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 7, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pSwapChainRenderTarget->mDesc.mWidth,
				(float)pSwapChainRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pSwapChainRenderTarget->mDesc.mWidth,
				pSwapChainRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesGatherBased.pPipelineComposite);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsGatherBased.pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsGatherBased.pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

			loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_LOAD;
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Draw UI", true);
			static HiresTimer gTimer;
			gTimer.GetUSec(true);

			#if defined(TARGET_IOS) || defined(__ANDROID__)
			gVirtualJoystick.Draw(cmd, { 1.0f, 1.0f, 1.0f, 1.0f });
			#endif

			gAppUI.DrawText(cmd, float2(8, 15), eastl::string().sprintf("CPU %f ms", gTimer.GetUSecAverage() / 1000.0f).c_str(), &gFrameTimeDraw);

			#if !defined(__ANDROID__)
			gAppUI.DrawText(
				cmd, float2(8, 40), eastl::string().sprintf("GPU %f ms", (float)pGpuProfiler->mCumulativeTime * 1000.0f).c_str(),
				&gFrameTimeDraw);
			gAppUI.DrawDebugGpuProfile(cmd, float2(8, 65), pGpuProfiler, NULL);
			#endif

			cmdDrawProfiler();

			gAppUI.Gui(pGui);

			gAppUI.Draw(cmd);
			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

			textureBarriers[0] = { pSwapChainRenderTarget->pTexture, RESOURCE_STATE_PRESENT };
			cmdResourceBarrier(cmd, 0, NULL, 1, textureBarriers);
		}
		cmdEndGpuFrameProfile(cmd, pGpuProfiler);
		endCmd(cmd);
		allCmds.push_back(cmd);
	}

	void RecordCommandBuffersSinglePass(eastl::vector<Cmd*>& allCmds)
	{
		// Load Actions
		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.depth = 1.0f;
		loadActions.mLoadActionStencil = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.stencil = 0;

		RenderTarget* pHdrRenderTarget = pRenderTargetHDR[gFrameIndex];
		RenderTarget* pGenCocRenderTarget = pRenderTargetCoC[gFrameIndex];

		Cmd* cmd = ppCmdsComposite[gFrameIndex];
		beginCmd(cmd);
		{
			RenderTarget* pSwapChainRenderTarget =
				pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "DOF Pass", true);

			TextureBarrier textureBarriers[3] =
			{
				{ pSwapChainRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 3, textureBarriers);

			loadActions = {};
			for (int i = 0; i < 1; ++i)
			{
				loadActions.mLoadActionsColor[i] = LOAD_ACTION_CLEAR;
				loadActions.mClearColorValues[i].r = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].g = 0.0f;
				loadActions.mClearColorValues[i].b = 0.0f;
				loadActions.mClearColorValues[i].a = 0.0f;
			}
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pSwapChainRenderTarget->mDesc.mWidth,
				(float)pSwapChainRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pSwapChainRenderTarget->mDesc.mWidth,
				pSwapChainRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, PipelinesSinglePass.pPipelineDOF);
			{
				cmdBindDescriptorSet(cmd, 0, DescriptorsSinglePass.pDescriptorSetsDOF[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, DescriptorsSinglePass.pDescriptorSetsDOF[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

			loadActions = {};
			loadActions.mLoadActionsColor[0] = LOAD_ACTION_LOAD;
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Draw UI", true);
			static HiresTimer gTimer;
			gTimer.GetUSec(true);

			#if defined(TARGET_IOS) || defined(__ANDROID__)
			gVirtualJoystick.Draw(cmd, { 1.0f, 1.0f, 1.0f, 1.0f });
			#endif

			gAppUI.DrawText(cmd, float2(8, 15), eastl::string().sprintf("CPU %f ms", gTimer.GetUSecAverage() / 1000.0f).c_str(), &gFrameTimeDraw);

			#if !defined(__ANDROID__)
			gAppUI.DrawText(
				cmd, float2(8, 40), eastl::string().sprintf("GPU %f ms", (float)pGpuProfiler->mCumulativeTime * 1000.0f).c_str(),
				&gFrameTimeDraw);
			gAppUI.DrawDebugGpuProfile(cmd, float2(8, 65), pGpuProfiler, NULL);
			#endif

			cmdDrawProfiler();

			gAppUI.Gui(pGui);

			gAppUI.Draw(cmd);
			cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

			textureBarriers[0] = { pSwapChainRenderTarget->pTexture, RESOURCE_STATE_PRESENT };
			cmdResourceBarrier(cmd, 0, NULL, 1, textureBarriers);
		}
		cmdEndGpuFrameProfile(cmd, pGpuProfiler);
		endCmd(cmd);
		allCmds.push_back(cmd);
	}

	void Draw()
	{
		eastl::vector<Cmd*> allCmds;

		acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL,
			&gFrameIndex);

		Semaphore* pRenderCompleteSemaphore = pRenderCompleteSemaphores[gFrameIndex];
		Fence* pRenderCompleteFence = pRenderCompleteFences[gFrameIndex];

		// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
		FenceStatus fenceStatus;
		getFenceStatus(pRenderer, pRenderCompleteFence, &fenceStatus);
		if (fenceStatus == FENCE_STATUS_INCOMPLETE)
			waitForFences(pRenderer, 1, &pRenderCompleteFence);

		// Update uniform buffers
		BufferUpdateDesc viewProjCbv = { pUniformBuffersProjView[gFrameIndex], &gUniformDataScene };
		updateResource(&viewProjCbv);

		BufferUpdateDesc uniformDOF = { pUniformBuffersDOF[gFrameIndex], &gUniformDataDOF };
		updateResource(&uniformDOF);

		// Update Light uniform buffers
		BufferUpdateDesc pointLightBuffUpdate = { pPointLightsBuffer, &pointLights };
		updateResource(&pointLightBuffUpdate);

		BufferUpdateDesc instanceDataUpdate = { pInstancePositionBuffer, &lightInstancedata.position };
		updateResource(&instanceDataUpdate);
		instanceDataUpdate = { pInstanceColorBuffer, &lightInstancedata.color };
		updateResource(&instanceDataUpdate);

		// Load Actions
		LoadActionsDesc loadActions = {};
		loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
		loadActions.mClearColorValues[0].r = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].g = 0.0f;
		loadActions.mClearColorValues[0].b = 0.0f;
		loadActions.mClearColorValues[0].a = 0.0f;
		loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.depth = 1.0f;
		loadActions.mLoadActionStencil = LOAD_ACTION_CLEAR;
		loadActions.mClearDepth.stencil = 0;

		RenderTarget* pSwapChainRenderTarget = pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

		RenderTarget* pHdrRenderTarget = pRenderTargetHDR[gFrameIndex];
		RenderTarget* pGenCocRenderTarget = pRenderTargetCoC[gFrameIndex];


		Cmd* cmd = ppCmdsHDR[gFrameIndex];
		beginCmd(cmd);
		cmdBeginGpuFrameProfile(cmd, pGpuProfiler);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Draw Scene Pass", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDepthBuffer->pTexture, RESOURCE_STATE_DEPTH_WRITE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			cmdBindRenderTargets(cmd, 1, &pHdrRenderTarget, pDepthBuffer,
				&loadActions, NULL, NULL, -1, -1);
			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pHdrRenderTarget->mDesc.mWidth,
				(float)pHdrRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pHdrRenderTarget->mDesc.mWidth,
				pHdrRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineScene);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);

				// Draw sponza

				struct MaterialMaps
				{
					uint mapIDs[1];
				} data;

				for (uint32_t i = 0; i < (uint32_t)gSponzaSceneData.MeshBatches.size(); ++i)
				{
					MeshBatch* mesh = gSponzaSceneData.MeshBatches[i];
					int materialID = mesh->MaterialID;
					materialID *= 5;

					#if !(defined(TARGET_IOS) || defined(__ANDROID__))
					data.mapIDs[0] = gSponzaTextureIndexforMaterial[materialID];
					cmdBindPushConstants(cmd, pRootSignatureScene, "cbTextureRootConstants", &data);
					#else
					cmdBindDescriptorSet(cmd, i, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_DRAW]);
					#endif

					Buffer* pVertexBuffers [] = { mesh->pPositionStream, mesh->pNormalStream, mesh->pUVStream };
					cmdBindVertexBuffer(cmd, 3, pVertexBuffers, NULL);

					cmdBindIndexBuffer(cmd, mesh->pIndicesStream, 0);

					cmdDrawIndexed(cmd, mesh->mCountIndices, 0, 0);
				}
			}

			cmdBindPipeline(cmd, pPipelineLight);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);

				Buffer* pVertexBuffers [] = { gLightMesh->pPositionStream, pInstancePositionBuffer, pInstanceColorBuffer };
				cmdBindVertexBuffer(cmd, 3, pVertexBuffers, NULL);
				cmdBindIndexBuffer(cmd, gLightMesh->pIndicesStream, 0);
				cmdDrawIndexedInstanced(cmd, gLightMesh->mCountIndices, 0, gPointLights, 0, 0);
			}

			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsCoC[gFrameIndex];
		beginCmd(cmd);
		cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Depth of Field", true);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Generate CoC Pass", true);

			TextureBarrier textureBarriers[2] =
			{
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDepthBuffer->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

			loadActions = {};
			cmdBindRenderTargets(cmd, 1, &pGenCocRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pGenCocRenderTarget->mDesc.mWidth,
				(float)pGenCocRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pGenCocRenderTarget->mDesc.mWidth,
				pGenCocRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineCoC);
			{
				cmdBindDescriptorSet(cmd, 0, pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		if (gSelectedMethod == 0)
			RecordCommandBuffersCircularDoF(allCmds);
		else if (gSelectedMethod == 1)
			RecordCommandBuffersGatherBased(allCmds);
		else if (gSelectedMethod == 2)
			RecordCommandBuffersSinglePass(allCmds);

		// Submit Second Pass Command Buffer
		queueSubmit(pGraphicsQueue, (uint32_t)allCmds.size(), allCmds.data(), pRenderCompleteFence, 1, &pImageAcquiredSemaphore, 1, &pRenderCompleteSemaphore);

		queuePresent(pGraphicsQueue, pSwapChain, gFrameIndex, 1, &pRenderCompleteSemaphore);

		flipProfiler();
	}

	bool addSwapChain()
	{
		SwapChainDesc swapChainDesc = {};
		swapChainDesc.mWindowHandle = pWindow->handle;
		swapChainDesc.mPresentQueueCount = 1;
		swapChainDesc.ppPresentQueues = &pGraphicsQueue;
		swapChainDesc.mImageCount = gImageCount;
		swapChainDesc.mSampleCount = SAMPLE_COUNT_1;
		swapChainDesc.mEnableVsync = false;
		swapChainDesc.mWidth = mSettings.mWidth;
		swapChainDesc.mHeight = mSettings.mHeight;
		swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true);
		::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
		return pSwapChain != NULL;
	}

	const char* GetName() { return "29_DepthOfField"; }

	// Pipeline State Objects

	void AddPipelineStateObjects()
	{
		// Create rasteriser state objects
		{
			RasterizerStateDesc rasterizerStateDesc = {};
			rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
			addRasterizerState(pRenderer, &rasterizerStateDesc, &pRasterDefault);
		}

		// Create depth state objects
		{
			DepthStateDesc depthStateDesc = {};
			depthStateDesc.mDepthTest = true;
			depthStateDesc.mDepthWrite = true;
			depthStateDesc.mDepthFunc = CMP_LEQUAL;
			addDepthState(pRenderer, &depthStateDesc, &pDepthDefault);

			depthStateDesc = {};
			depthStateDesc.mDepthTest = false;
			depthStateDesc.mDepthWrite = false;
			depthStateDesc.mDepthFunc = CMP_ALWAYS;
			addDepthState(pRenderer, &depthStateDesc, &pDepthNone);
		}
	}

	void RemovePipelineStateObjects()
	{
		removeRasterizerState(pRasterDefault);
		removeDepthState(pDepthDefault);
		removeDepthState(pDepthNone);
	}

	// Samplers

	void AddSamplers()
	{
		// Static Samplers
		SamplerDesc samplerDesc = { FILTER_LINEAR,
								   FILTER_LINEAR,
								   MIPMAP_MODE_LINEAR,
								   ADDRESS_MODE_REPEAT,
								   ADDRESS_MODE_REPEAT,
								   ADDRESS_MODE_REPEAT };
		addSampler(pRenderer, &samplerDesc, &pSamplerLinear);

		samplerDesc = { FILTER_LINEAR,
						FILTER_LINEAR,
						MIPMAP_MODE_LINEAR,
						ADDRESS_MODE_CLAMP_TO_EDGE,
						ADDRESS_MODE_CLAMP_TO_EDGE,
						ADDRESS_MODE_CLAMP_TO_EDGE };
		addSampler(pRenderer, &samplerDesc, &pSamplerLinearClampEdge);

		samplerDesc = { FILTER_NEAREST,
						FILTER_NEAREST,
						MIPMAP_MODE_NEAREST,
						ADDRESS_MODE_CLAMP_TO_EDGE,
						ADDRESS_MODE_CLAMP_TO_EDGE,
						ADDRESS_MODE_CLAMP_TO_EDGE };
		addSampler(pRenderer, &samplerDesc, &pSamplerPoint);

		samplerDesc = { FILTER_LINEAR,
						FILTER_LINEAR,
						MIPMAP_MODE_LINEAR,
						ADDRESS_MODE_REPEAT,
						ADDRESS_MODE_REPEAT,
						ADDRESS_MODE_REPEAT,
						0, 1.0f };
		addSampler(pRenderer, &samplerDesc, &pSamplerAnisotropic);
	}

	void RemoveSamplers()
	{
		removeSampler(pRenderer, pSamplerLinear);
		removeSampler(pRenderer, pSamplerLinearClampEdge);
		removeSampler(pRenderer, pSamplerPoint);
		removeSampler(pRenderer, pSamplerAnisotropic);
	}

	// Sync Objects

	void AddSyncObjects()
	{
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			addFence(pRenderer, &pRenderCompleteFences[i]);
			addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
		}
		addSemaphore(pRenderer, &pImageAcquiredSemaphore);
	}

	void RemoveSyncObjects()
	{
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeFence(pRenderer, pRenderCompleteFences[i]);
			removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);
		}
		removeSemaphore(pRenderer, pImageAcquiredSemaphore);
	}

	// Buffers

	void AddBuffers()
	{
		// Scene Uniform
		{
			BufferLoadDesc bufferDesc = {};
			bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
			bufferDesc.mDesc.mSize = sizeof(gUniformDataScene);
			bufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
			bufferDesc.pData = NULL;

			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				bufferDesc.ppBuffer = &pUniformBuffersProjView[i];
				addResource(&bufferDesc);
			}
		}

		// DOF Uniform
		{
			BufferLoadDesc bufferDesc = {};
			bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
			bufferDesc.mDesc.mSize = sizeof(gUniformDataScene);
			bufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
			bufferDesc.pData = NULL;

			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				bufferDesc.ppBuffer = &pUniformBuffersDOF[i];
				addResource(&bufferDesc);
			}
		}

		// PointLights Structured Buffer
		{
			BufferLoadDesc bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
			bufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_NONE;
			bufferDesc.mDesc.mSize = sizeof(PointLight) * gPointLights;
			bufferDesc.pData = NULL;
			bufferDesc.ppBuffer = &pPointLightsBuffer;
			addResource(&bufferDesc);
		}

		// Light Instance
		{
			BufferLoadDesc bufferDesc = {};
			bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
			bufferDesc.mDesc.mVertexStride = sizeof(float3);
			bufferDesc.mDesc.mSize = sizeof(float3) * gPointLights;
			bufferDesc.mDesc.mStartState = RESOURCE_STATE_GENERIC_READ;
			bufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
			bufferDesc.pData = NULL;
			bufferDesc.ppBuffer = &pInstancePositionBuffer;
			addResource(&bufferDesc);
			bufferDesc.ppBuffer = &pInstanceColorBuffer;
			addResource(&bufferDesc);
		}
	}

	void RemoveBuffers()
	{
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeResource(pUniformBuffersProjView[i]);
			removeResource(pUniformBuffersDOF[i]);
		}

		removeResource(pInstancePositionBuffer);
		removeResource(pInstanceColorBuffer);
		removeResource(pPointLightsBuffer);
	}

	// Shaders

	void AddShadersCircularDoF()
	{
		ShaderLoadDesc shaderDesc = {};
		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "circular_dof/downsample.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersCircularDoF.pShaderDownres);

		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "circular_dof/horizontalDof.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersCircularDoF.pShaderHorizontalDof);

		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "circular_dof/composite.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersCircularDoF.pShaderComposite);
	}

	void AddShadersGatherBased()
	{
		ShaderLoadDesc shaderDesc = {};
		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "gather_based_dof/downsample_2.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersGatherBased.pShaderDownres);

		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "gather_based_dof/computation.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersGatherBased.pShaderComputation);


		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "gather_based_dof/fill.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersGatherBased.pShaderFill);


		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "gather_based_dof/composite_2.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersGatherBased.pShaderComposite);
	}

	void AddShadersSinglePass()
	{
		ShaderLoadDesc shaderDesc = {};
		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "singlepass/dof.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &ShadersSinglePass.pShaderDOF);
	}

	void AddShaders()
	{
		ShaderLoadDesc shaderDesc = {};

		eastl::string num_lights_macro = eastl::string().sprintf("%i", (int)gPointLights);
		eastl::string total_imgs_macro = eastl::string().sprintf("%i", TOTAL_SPONZA_IMGS);

		ShaderMacro    sceneShaderMacros[2] =
		{
			{ "NUM_LIGHTS",  num_lights_macro.c_str() },
			{ "TOTAL_IMGS",  total_imgs_macro.c_str() }
		};
		shaderDesc.mStages[0] = { "meshes/sponza.vert", NULL, 0, RD_SHADER_SOURCES };

		#ifdef __ANDROID__
		shaderDesc.mStages[1] = { "meshes/sponza_Android.frag", sceneShaderMacros, 1, RD_SHADER_SOURCES };
		#elif TARGET_IOS
		//separate fragment gbuffer pass for iOs that does not use bindless textures
		shaderDesc.mStages[1] = { "meshes/sponza_iOS.frag", sceneShaderMacros, 1, RD_SHADER_SOURCES };
		#else
		shaderDesc.mStages[1] = { "meshes/sponza.frag", sceneShaderMacros, 2, RD_SHADER_SOURCES };
		#endif

		addShader(pRenderer, &shaderDesc, &pShaderBasic);


		shaderDesc.mStages[0] = { "meshes/light.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "meshes/light.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &pShaderLight);

		eastl::string near_float_macro = eastl::string().sprintf("%f", gNear);
		eastl::string far_float_macro = eastl::string().sprintf("%f", gFar);

		ShaderMacro    ShaderMacroCoC[2] =
		{
			{ "zNear", near_float_macro.c_str() },
			{ "zFar", far_float_macro.c_str() }
		};
		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "genCoc.frag", ShaderMacroCoC, 2, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &pShaderGenCoc);

		ShaderMacro    ShaderMacroMaxFilterCoC[1] =
		{
			{ "HORIZONTAL", "" },
		};
		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "maxfilterNearCoC.frag", ShaderMacroMaxFilterCoC, 1, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &pShaderMaxFilterNearCoCX);
		shaderDesc.mStages[1] = { "maxfilterNearCoC.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &pShaderMaxFilterNearCoCY);


		ShaderMacro    ShaderMacroBoxFilterCoC[1] =
		{
			{ "HORIZONTAL", "" },
		};
		shaderDesc.mStages[0] = { "image.vert", NULL, 0, RD_SHADER_SOURCES };
		shaderDesc.mStages[1] = { "boxfilterNearCoC.frag", ShaderMacroBoxFilterCoC, 1, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &pShaderBoxFilterNearCoCX);
		shaderDesc.mStages[1] = { "boxfilterNearCoC.frag", NULL, 0, RD_SHADER_SOURCES };
		addShader(pRenderer, &shaderDesc, &pShaderBoxFilterNearCoCY);

		AddShadersCircularDoF();
		AddShadersGatherBased();
		AddShadersSinglePass();
	}

	void RemoveShaders()
	{
		removeShader(pRenderer, pShaderBasic);
		removeShader(pRenderer, pShaderLight);
		removeShader(pRenderer, pShaderGenCoc);
		removeShader(pRenderer, pShaderMaxFilterNearCoCX);
		removeShader(pRenderer, pShaderMaxFilterNearCoCY);
		removeShader(pRenderer, pShaderBoxFilterNearCoCX);
		removeShader(pRenderer, pShaderBoxFilterNearCoCY);

		removeShader(pRenderer, ShadersCircularDoF.pShaderDownres);
		removeShader(pRenderer, ShadersCircularDoF.pShaderHorizontalDof);
		removeShader(pRenderer, ShadersCircularDoF.pShaderComposite);

		removeShader(pRenderer, ShadersGatherBased.pShaderDownres);
		removeShader(pRenderer, ShadersGatherBased.pShaderComputation);
		removeShader(pRenderer, ShadersGatherBased.pShaderFill);
		removeShader(pRenderer, ShadersGatherBased.pShaderComposite);

		removeShader(pRenderer, ShadersSinglePass.pShaderDOF);
	}

	// Descriptor Sets

	void AddDescriptorSetsCircularDoF()
	{
		// Downres
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint		numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersCircularDoF.pShaderDownres };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesCircularDoF.pRootSignatureDownres);
			}

			DescriptorSetDesc setDesc = { RootSignaturesCircularDoF.pRootSignatureDownres, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsCircularDoF.pDescriptorSetsDownres[0]);
			setDesc = { RootSignaturesCircularDoF.pRootSignatureDownres, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsCircularDoF.pDescriptorSetsDownres[1]);
		}

		// Horizontal DoF
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersCircularDoF.pShaderHorizontalDof };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesCircularDoF.pRootSignatureHorizontalPass);
			}

			DescriptorSetDesc setDesc = { RootSignaturesCircularDoF.pRootSignatureHorizontalPass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[0]);
			setDesc = { RootSignaturesCircularDoF.pRootSignatureHorizontalPass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[1]);
		}

		// Composite
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersCircularDoF.pShaderComposite };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesCircularDoF.pRootSignatureCompositePass);
			}

			DescriptorSetDesc setDesc = { RootSignaturesCircularDoF.pRootSignatureCompositePass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsCircularDoF.pDescriptorSetsCompositePass[0]);
			setDesc = { RootSignaturesCircularDoF.pRootSignatureCompositePass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsCircularDoF.pDescriptorSetsCompositePass[1]);
		}
	}

	void AddDescriptorSetsGatherBased()
	{
		// Downres
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersGatherBased.pShaderDownres };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesGatherBased.pRootSignatureDownres);
			}

			DescriptorSetDesc setDesc = { RootSignaturesGatherBased.pRootSignatureDownres, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsDownres[0]);
			setDesc = { RootSignaturesGatherBased.pRootSignatureDownres, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsDownres[1]);
		}

		// Computation
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersGatherBased.pShaderComputation };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesGatherBased.pRootSignatureComputationPass);
			}

			DescriptorSetDesc setDesc = { RootSignaturesGatherBased.pRootSignatureComputationPass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsComputationPass[0]);
			setDesc = { RootSignaturesGatherBased.pRootSignatureComputationPass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsComputationPass[1]);
		}

		// Filling
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersGatherBased.pShaderFill };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesGatherBased.pRootSignatureFillingPass);
			}

			DescriptorSetDesc setDesc = { RootSignaturesGatherBased.pRootSignatureFillingPass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsFillingPass[0]);
			setDesc = { RootSignaturesGatherBased.pRootSignatureFillingPass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsFillingPass[1]);
		}

		// Composite
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersGatherBased.pShaderComposite };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesGatherBased.pRootSignatureCompositePass);
			}

			DescriptorSetDesc setDesc = { RootSignaturesGatherBased.pRootSignatureCompositePass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsCompositePass[0]);
			setDesc = { RootSignaturesGatherBased.pRootSignatureCompositePass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsGatherBased.pDescriptorSetsCompositePass[1]);
		}
	}

	void AddDescriptorSetsSinglePass()
	{
		// DoF
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { ShadersSinglePass.pShaderDOF };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &RootSignaturesSinglePass.pRootSignatureDOF);
			}

			DescriptorSetDesc setDesc = { RootSignaturesSinglePass.pRootSignatureDOF, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsSinglePass.pDescriptorSetsDOF[0]);
			setDesc = { RootSignaturesSinglePass.pRootSignatureDOF, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &DescriptorsSinglePass.pDescriptorSetsDOF[1]);
		}
	}

	void AddDescriptorSets()
	{
		// HDR
		{
			const char* pStaticSamplerNames [] = { "samplerAnisotropic" };
			Sampler* pStaticSamplers [] = { pSamplerAnisotropic };
			uint        numStaticSamplers = 1;
			{
				Shader* shaders[2] = { pShaderBasic, pShaderLight };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 2;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				#if !(defined(TARGET_IOS) || defined(__ANDROID__))
				rootDesc.mMaxBindlessTextures = TOTAL_SPONZA_IMGS;
				#endif

				addRootSignature(pRenderer, &rootDesc, &pRootSignatureScene);
			}

			DescriptorSetDesc setDesc = { pRootSignatureScene, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsScene[0]);
			setDesc = { pRootSignatureScene, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsScene[1]);
		}

		// CoC
		{
			const char* pStaticSamplerNames [] = { "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerPoint };
			uint        numStaticSamplers = 1;
			{
				Shader* shaders[1] = { pShaderGenCoc };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureCoC);
			}

			DescriptorSetDesc setDesc = { pRootSignatureCoC, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsCoc[0]);
			setDesc = { pRootSignatureCoC, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsCoc[1]);
		}

		// MaxFilter Near CoC X
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderMaxFilterNearCoCX };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureMaxFilterNearX);
			}

			DescriptorSetDesc setDesc = { pRootSignatureMaxFilterNearX, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearMaxFilterX[0]);
			setDesc = { pRootSignatureMaxFilterNearX, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearMaxFilterX[1]);
		}

		// MaxFilter Near CoC Y
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderMaxFilterNearCoCY };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureMaxFilterNearY);
			}

			DescriptorSetDesc setDesc = { pRootSignatureMaxFilterNearY, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearMaxFilterY[0]);
			setDesc = { pRootSignatureMaxFilterNearY, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearMaxFilterY[1]);
		}

		// BoxFilter Near CoC X
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderBoxFilterNearCoCX };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureBoxFilterNearX);
			}

			DescriptorSetDesc setDesc = { pRootSignatureBoxFilterNearX, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearBoxFilterX[0]);
			setDesc = { pRootSignatureBoxFilterNearX, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearBoxFilterX[1]);
		}

		// BoxFilter Near CoC Y
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderBoxFilterNearCoCY };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureBoxFilterNearY);
			}

			DescriptorSetDesc setDesc = { pRootSignatureBoxFilterNearY, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearBoxFilterY[0]);
			setDesc = { pRootSignatureBoxFilterNearY, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsNearBoxFilterY[1]);
		}

		AddDescriptorSetsCircularDoF();
		AddDescriptorSetsGatherBased();
		AddDescriptorSetsSinglePass();
	}

	void RemoveDescriptorSets()
	{
		removeRootSignature(pRenderer, pRootSignatureScene);
		removeRootSignature(pRenderer, pRootSignatureCoC);
		removeRootSignature(pRenderer, pRootSignatureMaxFilterNearX);
		removeRootSignature(pRenderer, pRootSignatureMaxFilterNearY);
		removeRootSignature(pRenderer, pRootSignatureBoxFilterNearX);
		removeRootSignature(pRenderer, pRootSignatureBoxFilterNearY);

		removeRootSignature(pRenderer, RootSignaturesCircularDoF.pRootSignatureDownres);
		removeRootSignature(pRenderer, RootSignaturesCircularDoF.pRootSignatureHorizontalPass);
		removeRootSignature(pRenderer, RootSignaturesCircularDoF.pRootSignatureCompositePass);


		removeRootSignature(pRenderer, RootSignaturesGatherBased.pRootSignatureDownres);
		removeRootSignature(pRenderer, RootSignaturesGatherBased.pRootSignatureComputationPass);
		removeRootSignature(pRenderer, RootSignaturesGatherBased.pRootSignatureFillingPass);
		removeRootSignature(pRenderer, RootSignaturesGatherBased.pRootSignatureCompositePass);

		removeRootSignature(pRenderer, RootSignaturesSinglePass.pRootSignatureDOF);


		for (int i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
		{
			if (pDescriptorSetsScene[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsScene[i]);
			if (pDescriptorSetsCoc[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsCoc[i]);
			if (pDescriptorSetsNearMaxFilterX[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearMaxFilterX[i]);
			if (pDescriptorSetsNearMaxFilterY[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearMaxFilterY[i]);
			if (pDescriptorSetsNearBoxFilterX[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearBoxFilterX[i]);
			if (pDescriptorSetsNearBoxFilterY[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearBoxFilterY[i]);

			if (DescriptorsCircularDoF.pDescriptorSetsDownres[i])
				removeDescriptorSet(pRenderer, DescriptorsCircularDoF.pDescriptorSetsDownres[i]);
			if (DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[i])
				removeDescriptorSet(pRenderer, DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[i]);
			if (DescriptorsCircularDoF.pDescriptorSetsCompositePass[i])
				removeDescriptorSet(pRenderer, DescriptorsCircularDoF.pDescriptorSetsCompositePass[i]);

			if (DescriptorsGatherBased.pDescriptorSetsDownres[i])
				removeDescriptorSet(pRenderer, DescriptorsGatherBased.pDescriptorSetsDownres[i]);
			if (DescriptorsGatherBased.pDescriptorSetsComputationPass[i])
				removeDescriptorSet(pRenderer, DescriptorsGatherBased.pDescriptorSetsComputationPass[i]);
			if (DescriptorsGatherBased.pDescriptorSetsFillingPass[i])
				removeDescriptorSet(pRenderer, DescriptorsGatherBased.pDescriptorSetsFillingPass[i]);
			if (DescriptorsGatherBased.pDescriptorSetsCompositePass[i])
				removeDescriptorSet(pRenderer, DescriptorsGatherBased.pDescriptorSetsCompositePass[i]);

			if (DescriptorsSinglePass.pDescriptorSetsDOF[i])
				removeDescriptorSet(pRenderer, DescriptorsSinglePass.pDescriptorSetsDOF[i]);
		}
	}

	void PrepareDecriptotSetsCircularDoF()
	{
		// Downres
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[2] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoC[i]->pTexture;
				params[1].pName = "TextureColor";
				params[1].ppTextures = &pRenderTargetHDR[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					DescriptorsCircularDoF.pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 2,
					params);
			}
		}

		// 2-Component for Far Field and 1-Component for Near Field
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[5] = {};
				params[0].pName = "TextureColor";
				params[0].ppTextures = &pRenderTargetColorDownres[i]->pTexture;
				params[1].pName = "TextureCoC";
				params[1].ppTextures = &pRenderTargetCoCDowres[i]->pTexture;
				params[2].pName = "TextureNearCoC";
				params[2].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				params[3].pName = "TextureColorMulFar";
				params[3].ppTextures = &pRenderTargetMulFarDownres[i]->pTexture;
				params[4].pName = "UniformDOF";
				params[4].ppBuffers = &pUniformBuffersDOF[i];
				updateDescriptorSet(pRenderer, i,
					DescriptorsCircularDoF.pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 5,
					params);
			}
		}

		// Composite
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[11] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoC[i]->pTexture;
				params[1].pName = "TextureColor";
				params[1].ppTextures = &pRenderTargetHDR[i]->pTexture;
				params[2].pName = "UniformDOF";
				params[2].ppBuffers = &pUniformBuffersDOF[i];
				params[3].pName = "TextureFarR";
				params[3].ppTextures = &RenderTargetsCircularDoF.pRenderTargetFarR[i]->pTexture;
				params[4].pName = "TextureFarG";
				params[4].ppTextures = &RenderTargetsCircularDoF.pRenderTargetFarG[i]->pTexture;
				params[5].pName = "TextureFarB";
				params[5].ppTextures = &RenderTargetsCircularDoF.pRenderTargetFarB[i]->pTexture;
				params[6].pName = "TextureNearR";
				params[6].ppTextures = &RenderTargetsCircularDoF.pRenderTargetNearR[i]->pTexture;
				params[7].pName = "TextureNearG";
				params[7].ppTextures = &RenderTargetsCircularDoF.pRenderTargetNearG[i]->pTexture;
				params[8].pName = "TextureNearB";
				params[8].ppTextures = &RenderTargetsCircularDoF.pRenderTargetNearB[i]->pTexture;
				params[9].pName = "TextureNearCoC";
				params[9].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				params[10].pName = "TextureWeights";
				params[10].ppTextures = &RenderTargetsCircularDoF.pRenderTargetWeights[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					DescriptorsCircularDoF.pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 11,
					params);
			}
		}
	}

	void PrepareDecriptotSetsGatherBased()
	{
		// Downres
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[2] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoC[i]->pTexture;
				params[1].pName = "TextureColor";
				params[1].ppTextures = &pRenderTargetHDR[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					DescriptorsGatherBased.pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 2,
					params);
			}
		}

		// Computation
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[5] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoCDowres[i]->pTexture;
				params[1].pName = "TextureColor";
				params[1].ppTextures = &pRenderTargetColorDownres[i]->pTexture;
				params[2].pName = "TextureColorMulFar";
				params[2].ppTextures = &pRenderTargetMulFarDownres[i]->pTexture;
				params[3].pName = "TextureNearCoCBlurred";
				params[3].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				params[4].pName = "UniformDOF";
				params[4].ppBuffers = &pUniformBuffersDOF[i];
				updateDescriptorSet(pRenderer, i,
					DescriptorsGatherBased.pDescriptorSetsComputationPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 5,
					params);
			}
		}

		// Fill
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[4] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoCDowres[i]->pTexture;
				params[1].pName = "TextureNearCoCBlur";
				params[1].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				params[2].pName = "TextureFar";
				params[2].ppTextures = &RenderTargetsGatherBased.pRenderTargetFar[i]->pTexture;
				params[3].pName = "TextureNear";
				params[3].ppTextures = &RenderTargetsGatherBased.pRenderTargetNear[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					DescriptorsGatherBased.pDescriptorSetsFillingPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 4,
					params);
			}
		}

		// Composite
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[7] = {};
				params[0].pName = "TextureColor";
				params[0].ppTextures = &pRenderTargetHDR[i]->pTexture;
				params[1].pName = "TextureCoC";
				params[1].ppTextures = &pRenderTargetCoC[i]->pTexture;
				params[2].pName = "TextureCoC_x4";
				params[2].ppTextures = &pRenderTargetCoCDowres[i]->pTexture;
				params[3].pName = "TextureNearCoCBlur";
				params[3].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				params[4].pName = "TextureFar_x4";
				params[4].ppTextures = &RenderTargetsGatherBased.pRenderTargetFarFilled[i]->pTexture;
				params[5].pName = "TextureNear_x4";
				params[5].ppTextures = &RenderTargetsGatherBased.pRenderTargetNearFilled[i]->pTexture;
				params[6].pName = "UniformDOF";
				params[6].ppBuffers = &pUniformBuffersDOF[i];
				updateDescriptorSet(pRenderer, i,
					DescriptorsGatherBased.pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 7,
					params);
			}
		}
	}

	void PrepareDecriptotSetsSinglePass()
	{
		// DoF
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[3] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoC[i]->pTexture;
				params[1].pName = "TextureColor";
				params[1].ppTextures = &pRenderTargetHDR[i]->pTexture;
				params[2].pName = "UniformDOF";
				params[2].ppBuffers = &pUniformBuffersDOF[i];
				updateDescriptorSet(pRenderer, i,
					DescriptorsSinglePass.pDescriptorSetsDOF[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 3,
					params);
			}
		}
	}

	void PrepareDescriptorSets()
	{

		#if defined(TARGET_IOS) || defined(__ANDROID__)
		DescriptorSetDesc setDesc = { pRootSignatureScene, DESCRIPTOR_UPDATE_FREQ_PER_DRAW, (uint32_t)gSponzaSceneData.MeshBatches.size() };
		addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_DRAW]);
		#endif
		
		// HDR Scene
		{
			DescriptorData params[2] = {};
			params[0].pName = "cbPerProp";

			params[0].ppBuffers = &gSponzaSceneData.pConstantBuffer;
			#if !(defined(TARGET_IOS) || defined(__ANDROID__))
			params[1].pName = "textureMaps";
			params[1].ppTextures = pMaterialTexturesSponza;
			params[1].mCount = TOTAL_SPONZA_IMGS;
			updateDescriptorSet(pRenderer, 0, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_NONE], 2, params);
			#else
			updateDescriptorSet(pRenderer, 0, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_NONE], 1, params);


			for (uint32_t i = 0; i < (uint32_t)gSponzaSceneData.MeshBatches.size(); ++i)
			{
				MeshBatch* mesh = gSponzaSceneData.MeshBatches[i];
				int materialID = mesh->MaterialID * 5;

				//bind textures explicitely for iOS
				//we only use Albedo for the time being so just bind the albedo texture.
				//for (int j = 0; j < 5; ++j)
				{
					params[0].pName = pTextureName[0];    //Albedo texture name
					uint textureId = gSponzaTextureIndexforMaterial[materialID];
					params[0].ppTextures = &pMaterialTexturesSponza[textureId];
				}
				//TODO: If we use more than albedo on iOS we need to bind every texture manually and update
				//descriptor param count.
				//one descriptor param if using bindless textures
				updateDescriptorSet(pRenderer, i, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_DRAW], 1, params);
			}

			#endif

			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[3] = {};
				params[0].pName = "cbPerPass";
				params[0].ppBuffers = &pUniformBuffersProjView[i];
				params[1].pName = "PointLightsData";
				params[1].ppBuffers = &pPointLightsBuffer;
				updateDescriptorSet(pRenderer, i, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 2, params);
			}
		}

		// CoC
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[3] = {};
				params[0].pName = "UniformDOF";
				params[0].ppBuffers = &pUniformBuffersDOF[i];
				params[1].pName = "DepthTexture";
				params[1].ppTextures = &pDepthBuffer->pTexture;
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 2,
					params);
			}
		}

		// Near CoC Max Filter X
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[1] = {};
				params[0].pName = "NearCoCTexture";
				params[0].ppTextures = &pRenderTargetCoCDowres[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 1,
					params);
			}
		}

		// Near CoC Max Filter Y
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[1] = {};
				params[0].pName = "NearCoCTexture";
				params[0].ppTextures = &pRenderTargetFilterNearCoC[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 1,
					params);
			}
		}

		// Near CoC Box Filter X
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[1] = {};
				params[0].pName = "NearCoCTexture";
				params[0].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 1,
					params);
			}
		}

		// Near CoC Box Filter Y
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[1] = {};
				params[0].pName = "NearCoCTexture";
				params[0].ppTextures = &pRenderTargetFilterNearCoC[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 1,
					params);
			}
		}

		PrepareDecriptotSetsCircularDoF();
		PrepareDecriptotSetsGatherBased();
		PrepareDecriptotSetsSinglePass();
	}

	// Render Targets

	void AddRenderTargetsCirularDoF()
	{
		ClearValue clearVal = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Far, R, G, B Components
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			#if defined(__ANDROID__) || defined(TARGET_IOS)
			rtDesc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
			#else
			rtDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
			#endif
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetColorDownres[0]->mDesc.mWidth;
			rtDesc.mHeight =  pRenderTargetColorDownres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetFarR[i]);
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetFarG[i]);
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetFarB[i]);
			}
		}

		// Near, R, G, B Components
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			#if defined(__ANDROID__) || defined(TARGET_IOS)
			rtDesc.mFormat = TinyImageFormat_R8G8_UNORM;
			#else
			rtDesc.mFormat = TinyImageFormat_R16G16_SFLOAT;
			#endif
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetColorDownres[0]->mDesc.mWidth;
			rtDesc.mHeight =  pRenderTargetColorDownres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetNearR[i]);
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetNearG[i]);
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetNearB[i]);
			}
		}

		// Weights
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R16_SFLOAT;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetCoCDowres[0]->mDesc.mWidth;
			rtDesc.mHeight = pRenderTargetCoCDowres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsCircularDoF.pRenderTargetWeights[i]);
			}
		}
	}

	void AddRenderTargetsGatherBased()
	{
		ClearValue clearVal = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Far + Far Filled
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = pRenderTargetColorDownres[0]->mDesc.mFormat;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetColorDownres[0]->mDesc.mWidth;
			rtDesc.mHeight =  pRenderTargetColorDownres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsGatherBased.pRenderTargetFar[i]);
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsGatherBased.pRenderTargetFarFilled[i]);
			}
		}

		// Near + Near Filled
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = pRenderTargetColorDownres[0]->mDesc.mFormat;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetColorDownres[0]->mDesc.mWidth;
			rtDesc.mHeight =  pRenderTargetColorDownres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsGatherBased.pRenderTargetNear[i]);
				addRenderTarget(pRenderer, &rtDesc, &RenderTargetsGatherBased.pRenderTargetNearFilled[i]);
			}
		}
	}

	void AddRenderTargets()
	{
		ClearValue clearVal = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Depth Buffer
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue.depth = 1.0f;
			rtDesc.mClearValue.stencil = 0;
			rtDesc.mFormat = TinyImageFormat_D32_SFLOAT;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = mSettings.mWidth;
			rtDesc.mHeight = mSettings.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;
			addRenderTarget(pRenderer, &rtDesc, &pDepthBuffer);
		}

		// HDR
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = mSettings.mWidth;
			rtDesc.mHeight = mSettings.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetHDR[i]);
		}

		// Circle of Confusion Render Target
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R8G8_UNORM;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = mSettings.mWidth;
			rtDesc.mHeight = mSettings.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetCoC[i]);
		}

		// Circle of Confusion Render Target Downres
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = pRenderTargetCoC[0]->mDesc.mFormat;
			rtDesc.mDepth = 1;
			rtDesc.mWidth	= mSettings.mWidth / 2;
			rtDesc.mHeight	= mSettings.mHeight / 2;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetCoCDowres[i]);
		}

		// Color + Far Downres
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = pRenderTargetHDR[0]->mDesc.mFormat;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetCoCDowres[0]->mDesc.mWidth;
			rtDesc.mHeight = pRenderTargetCoCDowres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetColorDownres[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetMulFarDownres[i]);
			}
		}

		// Filter NearCoC
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R8_UNORM;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetCoCDowres[0]->mDesc.mWidth;
			rtDesc.mHeight = pRenderTargetCoCDowres[0]->mDesc.mWidth;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFilterNearCoC[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFilterNearCoCFinal[i]);
			}
		}

		AddRenderTargetsCirularDoF();
		AddRenderTargetsGatherBased();
	}

	void RemoveRenderTargets()
	{
		removeRenderTarget(pRenderer, pDepthBuffer);

		for (int i = 0; i < gImageCount; ++i)
		{
			removeRenderTarget(pRenderer, pRenderTargetHDR[i]);
			removeRenderTarget(pRenderer, pRenderTargetCoC[i]);
			removeRenderTarget(pRenderer, pRenderTargetCoCDowres[i]);
			removeRenderTarget(pRenderer, pRenderTargetColorDownres[i]);
			removeRenderTarget(pRenderer, pRenderTargetMulFarDownres[i]);

			removeRenderTarget(pRenderer, pRenderTargetFilterNearCoC[i]);
			removeRenderTarget(pRenderer, pRenderTargetFilterNearCoCFinal[i]);

			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetFarR[i]);
			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetFarG[i]);
			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetFarB[i]);
			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetNearR[i]);
			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetNearG[i]);
			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetNearB[i]);
			removeRenderTarget(pRenderer, RenderTargetsCircularDoF.pRenderTargetWeights[i]);

			removeRenderTarget(pRenderer, RenderTargetsGatherBased.pRenderTargetFar[i]);
			removeRenderTarget(pRenderer, RenderTargetsGatherBased.pRenderTargetFarFilled[i]);
			removeRenderTarget(pRenderer, RenderTargetsGatherBased.pRenderTargetNear[i]);
			removeRenderTarget(pRenderer, RenderTargetsGatherBased.pRenderTargetNearFilled[i]);
		}
	}

	// Pipelines

	void AddPipelinesCircularDoF()
	{
		// Downres
		{
			TinyImageFormat formats[3] =
			{
				pRenderTargetCoCDowres[0]->mDesc.mFormat,
				pRenderTargetColorDownres[0]->mDesc.mFormat,
				pRenderTargetMulFarDownres[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 3;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = pRenderTargetCoCDowres[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetCoCDowres[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesCircularDoF.pRootSignatureDownres;
			pipelineSettings.pShaderProgram = ShadersCircularDoF.pShaderDownres;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesCircularDoF.pPipelineDownres);
		}

		// Horizontal Filter
		{
			TinyImageFormat formats[7] =
			{
				RenderTargetsCircularDoF.pRenderTargetFarR[0]->mDesc.mFormat,
				RenderTargetsCircularDoF.pRenderTargetFarG[0]->mDesc.mFormat,
				RenderTargetsCircularDoF.pRenderTargetFarB[0]->mDesc.mFormat,
				RenderTargetsCircularDoF.pRenderTargetNearR[0]->mDesc.mFormat,
				RenderTargetsCircularDoF.pRenderTargetNearG[0]->mDesc.mFormat,
				RenderTargetsCircularDoF.pRenderTargetNearB[0]->mDesc.mFormat,
				RenderTargetsCircularDoF.pRenderTargetWeights[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 7;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = RenderTargetsCircularDoF.pRenderTargetFarR[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = RenderTargetsCircularDoF.pRenderTargetFarR[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesCircularDoF.pRootSignatureHorizontalPass;
			pipelineSettings.pShaderProgram = ShadersCircularDoF.pShaderHorizontalDof;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesCircularDoF.pPipelineHorizontalDOF);
		}

		// Composite
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = &pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesCircularDoF.pRootSignatureCompositePass;
			pipelineSettings.pShaderProgram = ShadersCircularDoF.pShaderComposite;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesCircularDoF.pPipelineComposite);
		}
	}

	void AddPipelinesGatherBased()
	{
		// Downres
		{
			TinyImageFormat formats[3] =
			{
				pRenderTargetCoCDowres[0]->mDesc.mFormat,
				pRenderTargetColorDownres[0]->mDesc.mFormat,
				pRenderTargetMulFarDownres[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 3;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = pRenderTargetCoCDowres[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetCoCDowres[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesGatherBased.pRootSignatureDownres;
			pipelineSettings.pShaderProgram = ShadersGatherBased.pShaderDownres;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesGatherBased.pPipelineDownres);
		}

		// Computation
		{
			TinyImageFormat formats[2] =
			{
				RenderTargetsGatherBased.pRenderTargetFar[0]->mDesc.mFormat,
				RenderTargetsGatherBased.pRenderTargetNear[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 2;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = RenderTargetsGatherBased.pRenderTargetFar[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = RenderTargetsGatherBased.pRenderTargetFar[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesGatherBased.pRootSignatureComputationPass;
			pipelineSettings.pShaderProgram = ShadersGatherBased.pShaderComputation;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesGatherBased.pPipelineComputation);
		}

		// Filling
		{
			TinyImageFormat formats[2] =
			{
				RenderTargetsGatherBased.pRenderTargetFarFilled[0]->mDesc.mFormat,
				RenderTargetsGatherBased.pRenderTargetNearFilled[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 2;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = RenderTargetsGatherBased.pRenderTargetFar[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = RenderTargetsGatherBased.pRenderTargetFar[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesGatherBased.pRootSignatureFillingPass;
			pipelineSettings.pShaderProgram = ShadersGatherBased.pShaderFill;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesGatherBased.pPipelineFilling);
		}

		// Composite
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = &pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesGatherBased.pRootSignatureCompositePass;
			pipelineSettings.pShaderProgram = ShadersGatherBased.pShaderComposite;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesGatherBased.pPipelineComposite);
		}
	}

	void AddPipelinesSinglePass()
	{
		// DoF
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = &pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = RootSignaturesSinglePass.pRootSignatureDOF;
			pipelineSettings.pShaderProgram = ShadersSinglePass.pShaderDOF;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &PipelinesSinglePass.pPipelineDOF);
		}
	}

	void AddPipelines()
	{
		// HDR Scene
		{
			VertexLayout vertexLayout = {};
			vertexLayout.mAttribCount = 3;

			vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
			vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
			vertexLayout.mAttribs[0].mBinding = 0;
			vertexLayout.mAttribs[0].mLocation = 0;
			vertexLayout.mAttribs[0].mOffset = 0;

			vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
			vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
			vertexLayout.mAttribs[1].mBinding = 1;
			vertexLayout.mAttribs[1].mLocation = 1;
			vertexLayout.mAttribs[1].mOffset = 0;

			vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
			vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32_SFLOAT;
			vertexLayout.mAttribs[2].mBinding = 2;
			vertexLayout.mAttribs[2].mLocation = 2;
			vertexLayout.mAttribs[2].mOffset = 0;

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthDefault;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats = &pRenderTargetHDR[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pRenderTargetHDR[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetHDR[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureScene;
			pipelineSettings.pShaderProgram = pShaderBasic;
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineScene);
		}

		// Render Light Bulbs
		{
			VertexLayout vertexLayout = {};
			vertexLayout.mAttribCount = 3;

			vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
			vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
			vertexLayout.mAttribs[0].mBinding = 0;
			vertexLayout.mAttribs[0].mLocation = 0;
			vertexLayout.mAttribs[0].mOffset = 0;

			vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
			vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
			vertexLayout.mAttribs[1].mRate = VERTEX_ATTRIB_RATE_INSTANCE;
			vertexLayout.mAttribs[1].mBinding = 1;
			vertexLayout.mAttribs[1].mLocation = 1;
			vertexLayout.mAttribs[1].mOffset = 0;

			vertexLayout.mAttribs[2].mSemantic = SEMANTIC_COLOR;
			vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
			vertexLayout.mAttribs[2].mRate = VERTEX_ATTRIB_RATE_INSTANCE;
			vertexLayout.mAttribs[2].mBinding = 2;
			vertexLayout.mAttribs[2].mLocation = 2;
			vertexLayout.mAttribs[2].mOffset = 0;

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthDefault;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats = &pRenderTargetHDR[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pRenderTargetHDR[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetHDR[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureScene;
			pipelineSettings.pShaderProgram = pShaderLight;
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineLight);
		}

		// CoC Gen
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = &pRenderTargetCoC[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pRenderTargetCoC[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetCoC[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureCoC;
			pipelineSettings.pShaderProgram = pShaderGenCoc;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineCoC);
		}

		// Filter Near CoC X
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = &pRenderTargetFilterNearCoC[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pRenderTargetFilterNearCoC[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetFilterNearCoC[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureMaxFilterNearX;
			pipelineSettings.pShaderProgram = pShaderMaxFilterNearCoCX;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineMaxFilterNearCoCX);

			pipelineSettings.pRootSignature = pRootSignatureBoxFilterNearX;
			pipelineSettings.pShaderProgram = pShaderBoxFilterNearCoCX;
			addPipeline(pRenderer, &desc, &pPipelineBoxFilterNearCoCX);
		}

		// Filter Near CoC Y
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.pColorFormats = &pRenderTargetFilterNearCoC[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pRenderTargetFilterNearCoC[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetFilterNearCoC[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureMaxFilterNearY;
			pipelineSettings.pShaderProgram = pShaderMaxFilterNearCoCY;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineMaxFilterNearCoCY);
			pipelineSettings.pRootSignature = pRootSignatureBoxFilterNearY;
			pipelineSettings.pShaderProgram = pShaderBoxFilterNearCoCY;
			addPipeline(pRenderer, &desc, &pPipelineBoxFilterNearCoCY);
		}

		AddPipelinesCircularDoF();
		AddPipelinesGatherBased();
		AddPipelinesSinglePass();
	}

	void RemovePipelines()
	{
		removePipeline(pRenderer, pPipelineScene);
		removePipeline(pRenderer, pPipelineLight);
		removePipeline(pRenderer, pPipelineCoC);
		removePipeline(pRenderer, pPipelineMaxFilterNearCoCX);
		removePipeline(pRenderer, pPipelineMaxFilterNearCoCY);
		removePipeline(pRenderer, pPipelineBoxFilterNearCoCX);
		removePipeline(pRenderer, pPipelineBoxFilterNearCoCY);

		removePipeline(pRenderer, PipelinesCircularDoF.pPipelineDownres);
		removePipeline(pRenderer, PipelinesCircularDoF.pPipelineHorizontalDOF);
		removePipeline(pRenderer, PipelinesCircularDoF.pPipelineComposite);

		removePipeline(pRenderer, PipelinesGatherBased.pPipelineDownres);
		removePipeline(pRenderer, PipelinesGatherBased.pPipelineComputation);
		removePipeline(pRenderer, PipelinesGatherBased.pPipelineFilling);
		removePipeline(pRenderer, PipelinesGatherBased.pPipelineComposite);

		removePipeline(pRenderer, PipelinesSinglePass.pPipelineDOF);
	}

	// Commands

	void AddCmds()
	{
		addCmdPool(pRenderer, pGraphicsQueue, false, &pCmdPool);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsHDR);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsCoC);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsDownress);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsMaxFilterNearCoCX);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsMaxFilterNearCoCY);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsBoxFilterNearCoCX);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsBoxFilterNearCoCY);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsComputation);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsFilling);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsComposite);
	}

	void RemoveCmds()
	{
		removeCmd_n(pCmdPool, gImageCount, ppCmdsHDR);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsCoC);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsDownress);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsMaxFilterNearCoCX);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsMaxFilterNearCoCY);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsBoxFilterNearCoCX);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsBoxFilterNearCoCY);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsComputation);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsFilling);
		removeCmd_n(pCmdPool, gImageCount, ppCmdsComposite);
		removeCmdPool(pRenderer, pCmdPool);
	}

	// Sponza

	void AssignSponzaTextures()
	{
		int AO = 5;
		int NoMetallic = 6;
		int Metallic = 7;

		//00 : leaf
		gSponzaTextureIndexforMaterial.push_back(66);
		gSponzaTextureIndexforMaterial.push_back(67);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(68);
		gSponzaTextureIndexforMaterial.push_back(AO);

		//01 : vase_round
		gSponzaTextureIndexforMaterial.push_back(78);
		gSponzaTextureIndexforMaterial.push_back(79);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(80);
		gSponzaTextureIndexforMaterial.push_back(AO);

		//02 : Material__57 (Plant)
		gSponzaTextureIndexforMaterial.push_back(75);
		gSponzaTextureIndexforMaterial.push_back(76);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(77);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 03 : Material__298
		gSponzaTextureIndexforMaterial.push_back(9);
		gSponzaTextureIndexforMaterial.push_back(10);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(11);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 04 : 16___Default (gi_flag)
		gSponzaTextureIndexforMaterial.push_back(8);
		gSponzaTextureIndexforMaterial.push_back(8);    // !!!!!!
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(8);    // !!!!!
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 05 : bricks
		gSponzaTextureIndexforMaterial.push_back(22);
		gSponzaTextureIndexforMaterial.push_back(23);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(24);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 06 :  arch
		gSponzaTextureIndexforMaterial.push_back(19);
		gSponzaTextureIndexforMaterial.push_back(20);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(21);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 07 : ceiling
		gSponzaTextureIndexforMaterial.push_back(25);
		gSponzaTextureIndexforMaterial.push_back(26);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(27);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 08 : column_a
		gSponzaTextureIndexforMaterial.push_back(28);
		gSponzaTextureIndexforMaterial.push_back(29);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(30);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 09 : Floor
		gSponzaTextureIndexforMaterial.push_back(60);
		gSponzaTextureIndexforMaterial.push_back(61);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(62);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 10 : column_c
		gSponzaTextureIndexforMaterial.push_back(34);
		gSponzaTextureIndexforMaterial.push_back(35);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(36);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 11 : details
		gSponzaTextureIndexforMaterial.push_back(45);
		gSponzaTextureIndexforMaterial.push_back(47);
		gSponzaTextureIndexforMaterial.push_back(46);
		gSponzaTextureIndexforMaterial.push_back(48);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 12 : column_b
		gSponzaTextureIndexforMaterial.push_back(31);
		gSponzaTextureIndexforMaterial.push_back(32);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(33);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 13 : Material__47 - it seems missing
		gSponzaTextureIndexforMaterial.push_back(19);
		gSponzaTextureIndexforMaterial.push_back(20);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(21);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 14 : flagpole
		gSponzaTextureIndexforMaterial.push_back(57);
		gSponzaTextureIndexforMaterial.push_back(58);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(59);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 15 : fabric_e (green)
		gSponzaTextureIndexforMaterial.push_back(51);
		gSponzaTextureIndexforMaterial.push_back(52);
		gSponzaTextureIndexforMaterial.push_back(53);
		gSponzaTextureIndexforMaterial.push_back(54);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 16 : fabric_d (blue)
		gSponzaTextureIndexforMaterial.push_back(49);
		gSponzaTextureIndexforMaterial.push_back(50);
		gSponzaTextureIndexforMaterial.push_back(53);
		gSponzaTextureIndexforMaterial.push_back(54);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 17 : fabric_a (red)
		gSponzaTextureIndexforMaterial.push_back(55);
		gSponzaTextureIndexforMaterial.push_back(56);
		gSponzaTextureIndexforMaterial.push_back(53);
		gSponzaTextureIndexforMaterial.push_back(54);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 18 : fabric_g (curtain_blue)
		gSponzaTextureIndexforMaterial.push_back(37);
		gSponzaTextureIndexforMaterial.push_back(38);
		gSponzaTextureIndexforMaterial.push_back(43);
		gSponzaTextureIndexforMaterial.push_back(44);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 19 : fabric_c (curtain_red)
		gSponzaTextureIndexforMaterial.push_back(41);
		gSponzaTextureIndexforMaterial.push_back(42);
		gSponzaTextureIndexforMaterial.push_back(43);
		gSponzaTextureIndexforMaterial.push_back(44);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 20 : fabric_f (curtain_green)
		gSponzaTextureIndexforMaterial.push_back(39);
		gSponzaTextureIndexforMaterial.push_back(40);
		gSponzaTextureIndexforMaterial.push_back(43);
		gSponzaTextureIndexforMaterial.push_back(44);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 21 : chain
		gSponzaTextureIndexforMaterial.push_back(12);
		gSponzaTextureIndexforMaterial.push_back(14);
		gSponzaTextureIndexforMaterial.push_back(13);
		gSponzaTextureIndexforMaterial.push_back(15);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 22 : vase_hanging
		gSponzaTextureIndexforMaterial.push_back(72);
		gSponzaTextureIndexforMaterial.push_back(73);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(74);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 23 : vase
		gSponzaTextureIndexforMaterial.push_back(69);
		gSponzaTextureIndexforMaterial.push_back(70);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(71);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 24 : Material__25 (lion)
		gSponzaTextureIndexforMaterial.push_back(16);
		gSponzaTextureIndexforMaterial.push_back(17);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(18);
		gSponzaTextureIndexforMaterial.push_back(AO);

		// 25 : roof
		gSponzaTextureIndexforMaterial.push_back(63);
		gSponzaTextureIndexforMaterial.push_back(64);
		gSponzaTextureIndexforMaterial.push_back(NoMetallic);
		gSponzaTextureIndexforMaterial.push_back(65);
		gSponzaTextureIndexforMaterial.push_back(AO);
	}

	bool LoadSponza()
	{
		//load Sponza
		//eastl::vector<Image> toLoad(TOTAL_IMGS);
		//adding material textures
		for (int i = 0; i < TOTAL_SPONZA_IMGS; ++i)
		{
			PathHandle texturePath = fsCopyPathInResourceDirectory(RD_TEXTURES, pMaterialImageFileNames[i]);
			TextureLoadDesc textureDesc = {};
			textureDesc.pFilePath = texturePath;
			textureDesc.ppTexture = &pMaterialTexturesSponza[i];
			addResource(&textureDesc, true);
		}
		AssignSponzaTextures();


		// Update Instance Data
		gSponzaSceneData.WorldMatrix =
			mat4::identity() *
			mat4::scale(Vector3(0.1f));

		//set constant buffer for sponza
		{
			cbPerObj data = {};
			data.mWorldMat = gSponzaSceneData.WorldMatrix;

			BufferLoadDesc desc = {};
			desc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			desc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
			desc.mDesc.mSize = sizeof(cbPerObj);
			desc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
			desc.pData = &data;
			desc.ppBuffer = &gSponzaSceneData.pConstantBuffer;
			addResource(&desc);
		}

		AssimpImporter importer;

		AssimpImporter::Model gModel_Sponza;
		PathHandle sceneFullPath = fsCopyPathInResourceDirectory(RD_MESHES, gModel_Sponza_File);
		if (!importer.ImportModel(sceneFullPath, &gModel_Sponza))
		{
			LOGF(LogLevel::eERROR, "Failed to load %s", fsGetPathFileName(sceneFullPath).buffer);
			finishResourceLoading();
			return false;
		}

		size_t meshCount = gModel_Sponza.mMeshArray.size();
		size_t sponza_matCount = gModel_Sponza.mMaterialList.size();

		for (int i = 0; i < meshCount; i++)
		{
			//skip the large cloth mid-scene
			if (i == 4)
				continue;

			AssimpImporter::Mesh subMesh = gModel_Sponza.mMeshArray[i];

			MeshBatch* pMeshBatch = (MeshBatch*)conf_placement_new<MeshBatch>(conf_calloc(1, sizeof(MeshBatch)));

			gSponzaSceneData.MeshBatches.push_back(pMeshBatch);

			pMeshBatch->MaterialID = subMesh.mMaterialId;
			pMeshBatch->mCountIndices = (int)subMesh.mIndices.size();
			pMeshBatch->mCountVertices = (int)subMesh.mPositions.size();

			for (uint32_t j = 0; j < pMeshBatch->mCountIndices; j++)
			{
				pMeshBatch->IndicesData.push_back(subMesh.mIndices[j]);
			}

			for (uint32_t j = 0; j < pMeshBatch->mCountVertices; j++)
			{
				pMeshBatch->PositionsData.push_back(subMesh.mPositions[j]);
			}

			// Vertex buffers for sponza
			{
				BufferLoadDesc desc = {};
				desc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
				desc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
				desc.mDesc.mVertexStride = sizeof(float3);
				desc.mDesc.mSize = subMesh.mPositions.size() * desc.mDesc.mVertexStride;
				desc.pData = subMesh.mPositions.data();
				desc.ppBuffer = &pMeshBatch->pPositionStream;
				addResource(&desc);

				desc.mDesc.mVertexStride = sizeof(float3);
				desc.mDesc.mSize = subMesh.mNormals.size() * desc.mDesc.mVertexStride;
				desc.pData = subMesh.mNormals.data();
				desc.ppBuffer = &pMeshBatch->pNormalStream;
				addResource(&desc);

				desc.mDesc.mVertexStride = sizeof(float2);
				desc.mDesc.mSize = subMesh.mUvs.size() * desc.mDesc.mVertexStride;
				desc.pData = subMesh.mUvs.data();
				desc.ppBuffer = &pMeshBatch->pUVStream;
				addResource(&desc);
			}

			// Index buffer for sponza
			{
				// Index buffer for the scene
				BufferLoadDesc desc = {};
				desc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
				desc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
				desc.mDesc.mIndexType = INDEX_TYPE_UINT32;
				desc.mDesc.mSize = sizeof(uint) * (uint)subMesh.mIndices.size();
				desc.pData = subMesh.mIndices.data();
				desc.ppBuffer = &pMeshBatch->pIndicesStream;
				addResource(&desc);
			}
		}

		{
			AssimpImporter::Model sphereModel;

			PathHandle sceneFullPath = fsCopyPathInResourceDirectory(RD_MESHES, gModel_Light_File);
			if (!importer.ImportModel(sceneFullPath, &sphereModel))
			{
				LOGF(LogLevel::eERROR, "Failed to load %s", fsGetPathFileName(sceneFullPath).buffer);
				finishResourceLoading();
				return false;
			}

			size_t meshSize = sphereModel.mMeshArray.size();

			AssimpImporter::Mesh subMesh = sphereModel.mMeshArray[0];

			MeshBatch* pMeshBatch = (MeshBatch*)conf_placement_new<MeshBatch>(conf_calloc(1, sizeof(MeshBatch)));

			gLightMesh = pMeshBatch;

			// Vertex Buffer
			BufferLoadDesc bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
			bufferDesc.mDesc.mVertexStride = sizeof(float3);
			bufferDesc.mDesc.mSize = bufferDesc.mDesc.mVertexStride * subMesh.mPositions.size();
			bufferDesc.pData = subMesh.mPositions.data();
			bufferDesc.ppBuffer = &pMeshBatch->pPositionStream;
			addResource(&bufferDesc);

			pMeshBatch->mCountIndices = (uint32_t)subMesh.mIndices.size();

			// Index buffer
			bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
			bufferDesc.mDesc.mIndexType = INDEX_TYPE_UINT32;
			bufferDesc.mDesc.mSize = sizeof(uint) * (uint)subMesh.mIndices.size();
			bufferDesc.pData = subMesh.mIndices.data();
			bufferDesc.ppBuffer = &pMeshBatch->pIndicesStream;
			addResource(&bufferDesc);
		}

		return true;
	}

	void UnloadSponza()
	{
		// Delete Sponza resources
		for (MeshBatch* meshBatch : gSponzaSceneData.MeshBatches)
		{
			removeResource(meshBatch->pIndicesStream);
			removeResource(meshBatch->pNormalStream);
			removeResource(meshBatch->pPositionStream);
			removeResource(meshBatch->pUVStream);
			meshBatch->~MeshBatch();
			conf_free(meshBatch);
		}

		removeResource(gSponzaSceneData.pConstantBuffer);
		gSponzaSceneData.MeshBatches.empty();
		gSponzaSceneData.MeshBatches.set_capacity(0);

		for (uint i = 0; i < TOTAL_SPONZA_IMGS; ++i)
			removeResource(pMaterialTexturesSponza[i]);

		gSponzaTextureIndexforMaterial.empty();
		gSponzaTextureIndexforMaterial.set_capacity(0);

		removeResource(gLightMesh->pPositionStream);
		removeResource(gLightMesh->pIndicesStream);
		gLightMesh->~MeshBatch();
		conf_free(gLightMesh);
	}
};

DEFINE_APPLICATION_MAIN(DepthOfField)
