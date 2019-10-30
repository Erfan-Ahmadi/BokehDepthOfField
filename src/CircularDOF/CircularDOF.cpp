#include "../common.h"

#include "Common_3/OS/Interfaces/IProfiler.h"
#include "Middleware_3/UI/AppUI.h"

#include "Common_3/ThirdParty/OpenSource/Nothings/stb_image.h"

#include "Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "Common_3/ThirdParty/OpenSource/EASTL/vector.h"

#include "Common_3/OS/Interfaces/IMemory.h"
#include "Common_3/Tools/AssimpImporter/AssimpImporter.h"

//--------------------------------------------------------------------------------------------
// GLOBAL DEFINTIONS
//--------------------------------------------------------------------------------------------

uint32_t gFrameIndex = 0;

const uint32_t gImageCount			= 3;

bool bToggleMicroProfiler			= false;
bool bPrevToggleMicroProfiler		= false;

constexpr float gNear				= 0.1f;
constexpr float gFar				= 300.0f;

static float gFocalPlaneDistance	= 60;
static float gFocalTransitionRange	= 50;

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
	float blend			= 7.0f;
	float nb			= gNear;
	float ne			= 0.0f;
	float fb			= 0.0f;
	float fe			= gFar;
	float2 projParams	= {};
};

struct PointLight
{
	#ifdef VULKAN
	alignas(16) float3 position;
	alignas(16) float3 ambient;
	alignas(16) float3 diffuse;
	alignas(16) float3 specular;
	alignas(16) float3 attenuationParams;
	#elif DIRECT3D12
	alignas(16) float3 position;
	alignas(16) float3 ambient;
	alignas(16) float3 diffuse;
	alignas(16) float3 specular;
	alignas(16) float3 attenuationParams;
	float _pad0;
	#endif
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
Cmd** ppCmdsHorizontalDof 														= NULL;
Cmd** ppCmdsComposite															= NULL;

SwapChain* pSwapChain 															= NULL;

Fence* pRenderCompleteFences[gImageCount] 									= { NULL };

Semaphore* pImageAcquiredSemaphore 												= NULL;
Semaphore* pRenderCompleteSemaphores[gImageCount] 								= { NULL };

Pipeline* pPipelineScene 														= NULL;
Pipeline* pPipelineLight 														= NULL;
Pipeline* pPipelineCoC 															= NULL;
Pipeline* pPipelineDownres 														= NULL;
Pipeline* pPipelineMaxFilterNearCoCX 											= NULL;
Pipeline* pPipelineMaxFilterNearCoCY 											= NULL;
Pipeline* pPipelineBoxFilterNearCoCX 											= NULL;
Pipeline* pPipelineBoxFilterNearCoCY 											= NULL;
Pipeline* pPipelineHorizontalDOF 												= NULL;
Pipeline* pPipelineComposite 													= NULL;

Shader* pShaderBasic 															= NULL;
Shader* pShaderLight 															= NULL;
Shader* pShaderGenCoc 															= NULL;
Shader* pShaderDownres 															= NULL;
Shader* pShaderMaxFilterNearCoCX 												= NULL;
Shader* pShaderMaxFilterNearCoCY 												= NULL;
Shader* pShaderBoxFilterNearCoCX 												= NULL;
Shader* pShaderBoxFilterNearCoCY 												= NULL;
Shader* pShaderHorizontalDof 													= NULL;
Shader* pShaderComposite 														= NULL;

DescriptorSet* pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_COUNT] 				= { NULL };
DescriptorSet* pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_COUNT] 				= { NULL };
DescriptorSet* pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_COUNT] 			= { NULL };
DescriptorSet* pDescriptorSetsNearMaxFilterX[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsNearMaxFilterY[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsNearBoxFilterX[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };
DescriptorSet* pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_COUNT] 		= { NULL };

RootSignature* pRootSignatureScene 												= NULL;
RootSignature* pRootSignatureCoC 												= NULL;
RootSignature* pRootSignatureDownres 											= NULL;
RootSignature* pRootSignatureMaxFilterNearX 									= NULL;
RootSignature* pRootSignatureMaxFilterNearY 									= NULL;
RootSignature* pRootSignatureBoxFilterNearX 									= NULL;
RootSignature* pRootSignatureBoxFilterNearY 									= NULL;
RootSignature* pRootSignatureHorizontalPass										= NULL;
RootSignature* pRootSignatureCompositePass 										= NULL;

Sampler* pSamplerLinear;
Sampler* pSamplerLinearClampEdge;
Sampler* pSamplerPoint;
Sampler* pSamplerAnisotropic;

RenderTarget* pDepthBuffer;

RenderTarget* pRenderTargetHDR[gImageCount]										= { NULL }; // R16G16B16A16
RenderTarget* pRenderTargetCoC[gImageCount]										= { NULL }; // R8G8

RenderTarget* pRenderTargetCoCDowres[gImageCount]								= { NULL }; // COC/4
RenderTarget* pRenderTargetColorDownres[gImageCount]							= { NULL };	// HDR/4

RenderTarget* pRenderTargetFilterNearCoC[gImageCount]							= { NULL };	// R8/4
RenderTarget* pRenderTargetFilterNearCoCFinal[gImageCount]						= { NULL };	// R8/4

RenderTarget* pRenderTargetFarR[gImageCount]									= { NULL }; // FAR/4
RenderTarget* pRenderTargetFarG[gImageCount]									= { NULL }; // FAR/4
RenderTarget* pRenderTargetFarB[gImageCount]									= { NULL }; // FAR/4

RenderTarget* pRenderTargetNearR[gImageCount]									= { NULL }; // NEAR/4
RenderTarget* pRenderTargetNearG[gImageCount]									= { NULL }; // NEAR/4
RenderTarget* pRenderTargetNearB[gImageCount]									= { NULL }; // NEAR/4

RenderTarget* pRenderTargetWeights[gImageCount]									= { NULL };

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

#define TOTAL_SPONZA_IMGS 84

Texture* pMaterialTexturesSponza[TOTAL_SPONZA_IMGS]							= { NULL };

eastl::vector<int>	gSponzaTextureIndexforMaterial;

const char* gModel_Sponza_File = "sponza.obj";

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

	// 90
	"lion/lion_albedo",
	"lion/lion_specular",
	"lion/lion_normal",

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

const char* pszBases[FSR_Count] =
{
	"../../../../src/Shaders/bin",													// FSR_BinShaders
	"../../../../src/CircularDOF/",													// FSR_SrcShaders
	"../../../../art/",																// FSR_Textures
	"../../../../art/",																// FSR_Meshes
	"../../../../../The-Forge/Examples_3/Unit_Tests/UnitTestResources/",			// FSR_Builtin_Fonts
	"../../../../../The-Forge/Examples_3/Unit_Tests/src/01_Transformations/",		// FSR_GpuConfig
	"",																				// FSR_Animation
	"",																				// FSR_Audio
	"",																				// FSR_OtherFiles
	"../../../../../The-Forge/Middleware_3/Text/",									// FSR_MIDDLEWARE_TEXT
	"../../../../../The-Forge/Middleware_3/UI/",									// FSR_MIDDLEWARE_UI
};

class CircularDOF: public IApp
{
	public:
	CircularDOF()
	{
		mSettings.mWidth = 1280;
		mSettings.mHeight = 720;
	}

	bool Init()
	{
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

		gAppUI.LoadFont("TitilliumText/TitilliumText-Bold.otf", FSR_Builtin_Fonts);

		GuiDesc guiDesc = {};
		float   dpiScale = getDpiScale().x;
		guiDesc.mStartSize = vec2(140.0f, 320.0f);
		guiDesc.mStartPosition = vec2(mSettings.mWidth / dpiScale - guiDesc.mStartSize.getX() * 1.1f, guiDesc.mStartSize.getY() * 0.5f);

		pGui = gAppUI.AddGuiComponent("Micro profiler", &guiDesc);

		pGui->AddWidget(CheckboxWidget("Toggle Micro Profiler", &bToggleMicroProfiler));
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
				gVirtualJoystick.OnMove(index, ctx->mPhase != INPUT_ACTION_PHASE_CANCELED, ctx->pPosition);
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

		gVirtualJoystick.Exit();

		gAppUI.Exit();

		exitProfiler();

		UnloadSponza();

		RemoveBuffers();

		RemoveDescriptorSets();

		RemoveSamplers();

		RemoveShaders();

		RemovePipelineStateObjects();

		RemoveSyncObjects();

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

		if (!gVirtualJoystick.Load(pSwapChain->ppSwapchainRenderTargets[0]))
			return false;

		loadProfiler(&gAppUI, mSettings.mWidth, mSettings.mHeight);

		AddPipelines();

		pCameraController->moveTo(vec3(0, 89, 0));
		pCameraController->lookAt(vec3(-5, 87, 0));

		PrepareDescriptorSets();

		for (int i = 0; i < gPointLights; ++i)
		{
			pointLights[i].attenuationParams = float3 { 1.0f,  0.045f,  0.0075f };
			pointLights[i].ambient = float3 { 0.1f, 0.1f, 0.1f };
			pointLights[i].position = float3 { -62.0f + 37.5f * (i / 2), 87.8f,  (i % 2) ? 8.5f : -1.5f };

			pointLights[i].diffuse = float3 {
				3.0f * (rand() % 255) / 255.0f,
				3.0f * (rand() % 255) / 255.0f,
				3.0f * (rand() % 255) / 255.0f };

			pointLights[i].specular = pointLights[i].diffuse;
		}

		return true;
	}

	void Unload()
	{
		waitQueueIdle(pGraphicsQueue);

		unloadProfiler();
		gAppUI.Unload();

		gVirtualJoystick.Unload();

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

		RenderTarget* pDownresRenderTargets[2] =
		{
			pRenderTargetCoCDowres[gFrameIndex],
			pRenderTargetColorDownres[gFrameIndex],
		};

		RenderTarget* pFilteredNearCoC = pRenderTargetFilterNearCoC[gFrameIndex];
		RenderTarget* pFilteredNearCoCFinal = pRenderTargetFilterNearCoCFinal[gFrameIndex];

		RenderTarget* pHorizontalRenderTargets[7] =
		{
			pRenderTargetFarR[gFrameIndex],
			pRenderTargetFarG[gFrameIndex],
			pRenderTargetFarB[gFrameIndex],
			pRenderTargetNearR[gFrameIndex],
			pRenderTargetNearG[gFrameIndex],
			pRenderTargetNearB[gFrameIndex],
			pRenderTargetWeights[gFrameIndex],
		};


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

			cmdBindDescriptorSet(cmd, 0, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_NONE]);
			cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsScene[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);

			cmdBindPipeline(cmd, pPipelineScene);
			{
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

					data.mapIDs[0] = gSponzaTextureIndexforMaterial[materialID];

					cmdBindPushConstants(cmd, pRootSignatureScene, "cbTextureRootConstants", &data);

					Buffer* pVertexBuffers [] = { mesh->pPositionStream, mesh->pNormalStream, mesh->pUVStream };
					cmdBindVertexBuffer(cmd, 3, pVertexBuffers, NULL);

					cmdBindIndexBuffer(cmd, mesh->pIndicesStream, 0);

					cmdDrawIndexed(cmd, mesh->mCountIndices, 0, 0);
				}
			}

			cmdBindPipeline(cmd, pPipelineLight);
			{
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
				//cmdBindDescriptorSet(cmd, 0, pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsCoc[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsDownress[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Downres Pass", true);

			TextureBarrier textureBarriers[4] =
			{
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pDownresRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 4, textureBarriers);

			loadActions = {};
			cmdBindRenderTargets(cmd, 2, pDownresRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pDownresRenderTargets[0]->mDesc.mWidth,
				(float)pDownresRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pDownresRenderTargets[0]->mDesc.mWidth,
				pDownresRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineDownres);
			{
				//cmdBindDescriptorSet(cmd, 0, pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
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
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoC, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoC->mDesc.mWidth,
				(float)pFilteredNearCoC->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoC->mDesc.mWidth,
				pFilteredNearCoC->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineMaxFilterNearCoCX);
			{
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
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoCFinal, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoCFinal->mDesc.mWidth,
				(float)pFilteredNearCoCFinal->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoCFinal->mDesc.mWidth,
				pFilteredNearCoCFinal->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineMaxFilterNearCoCY);
			{
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
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoC, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoC->mDesc.mWidth,
				(float)pFilteredNearCoC->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoC->mDesc.mWidth,
				pFilteredNearCoC->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineBoxFilterNearCoCX);
			{
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
			cmdBindRenderTargets(cmd, 1, &pFilteredNearCoCFinal, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pFilteredNearCoCFinal->mDesc.mWidth,
				(float)pFilteredNearCoCFinal->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pFilteredNearCoCFinal->mDesc.mWidth,
				pFilteredNearCoCFinal->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineBoxFilterNearCoCY);
			{
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsNearBoxFilterY[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
				cmdDraw(cmd, 3, 0);
			}
			cmdEndGpuTimestampQuery(cmd, pGpuProfiler);
		}
		endCmd(cmd);
		allCmds.push_back(cmd);

		cmd = ppCmdsHorizontalDof[gFrameIndex];
		beginCmd(cmd);
		{
			cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Horizontal Blur Pass", true);

			TextureBarrier textureBarriers[9] =
			{
				{ pHorizontalRenderTargets[0]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[1]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[2]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[3]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[4]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[5]->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },	// NearCoC DownRes
				{ pDownresRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // CoC DownRes
				{ pDownresRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE }, // Color DownRes
			};

			cmdResourceBarrier(cmd, 0, nullptr, 9, textureBarriers);

			loadActions = {};

			cmdBindRenderTargets(cmd, 6, pHorizontalRenderTargets, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pHorizontalRenderTargets[0]->mDesc.mWidth,
				(float)pHorizontalRenderTargets[0]->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pHorizontalRenderTargets[0]->mDesc.mWidth,
				pHorizontalRenderTargets[0]->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineHorizontalDOF);
			{
				//cmdBindDescriptorSet(cmd, 0, pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
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

			TextureBarrier textureBarriers[10] =
			{
				{ pSwapChainRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET },
				{ pHorizontalRenderTargets[0]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[1]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[2]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[3]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[4]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHorizontalRenderTargets[5]->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pFilteredNearCoCFinal->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pGenCocRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE },
				{ pHdrRenderTarget->pTexture, RESOURCE_STATE_SHADER_RESOURCE }
			};

			cmdResourceBarrier(cmd, 0, nullptr, 10, textureBarriers);

			loadActions = {};
			cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);

			cmdSetViewport(
				cmd, 0.0f, 0.0f, (float)pSwapChainRenderTarget->mDesc.mWidth,
				(float)pSwapChainRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
			cmdSetScissor(cmd, 0, 0, pSwapChainRenderTarget->mDesc.mWidth,
				pSwapChainRenderTarget->mDesc.mHeight);

			cmdBindPipeline(cmd, pPipelineComposite);
			{
				//cmdBindDescriptorSet(cmd, 0, pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_NONE]);
				cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);
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

			gVirtualJoystick.Draw(cmd, { 1.0f, 1.0f, 1.0f, 1.0f });

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

		// Submit Second Pass Command Buffer
		queueSubmit(pGraphicsQueue, allCmds.size(), allCmds.data(), pRenderCompleteFence, 1, &pImageAcquiredSemaphore, 1, &pRenderCompleteSemaphore);

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

	const char* GetName() { return "CircularDOF"; }

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
		removeResource(pPointLightsBuffer);
	}

	// Shaders

	void AddShaders()
	{
		ShaderMacro    sceneShaderMacros[2] =
		{
			{ "TOTAL_IMGS", eastl::string().sprintf("%i", TOTAL_SPONZA_IMGS) },
			{ "NUM_LIGHTS", eastl::string().sprintf("%i", gPointLights) }
		};
		ShaderLoadDesc shaderDesc = {};
		shaderDesc.mStages[0] = { "meshes/basic.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "meshes/basic.frag", sceneShaderMacros, 2, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderBasic);

		shaderDesc.mStages[0] = { "meshes/light.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "meshes/light.frag", NULL, 0, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderLight);

		ShaderMacro    ShaderMacroCoC[2] =
		{
			{ "zNear", eastl::string().sprintf("%f", gNear) },
			{ "zFar", eastl::string().sprintf("%f", gFar) }
		};
		shaderDesc.mStages[0] = { "dof/image.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "dof/genCoc.frag", ShaderMacroCoC, 2, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderGenCoc);

		shaderDesc.mStages[0] = { "dof/image.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "dof/downsample.frag", NULL, 0, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderDownres);

		ShaderMacro    ShaderMacroMaxFilterCoC[1] =
		{
			{ "HORIZONTAL", eastl::string().sprintf("true") },
		};
		shaderDesc.mStages[0] = { "dof/image.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "dof/maxfilterNearCoC.frag", ShaderMacroMaxFilterCoC, 1, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderMaxFilterNearCoCX);
		ShaderMacroMaxFilterCoC[0] = { "HORIZONTAL", eastl::string().sprintf("false") };
		addShader(pRenderer, &shaderDesc, &pShaderMaxFilterNearCoCY);


		ShaderMacro    ShaderMacroBoxFilterCoC[1] =
		{
			{ "HORIZONTAL", eastl::string().sprintf("true") },
		};
		shaderDesc.mStages[0] = { "dof/image.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "dof/boxfilterNearCoC.frag", ShaderMacroBoxFilterCoC, 1, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderBoxFilterNearCoCX);
		ShaderMacroBoxFilterCoC[0] = { "HORIZONTAL", eastl::string().sprintf("false") };
		addShader(pRenderer, &shaderDesc, &pShaderBoxFilterNearCoCY);

		shaderDesc.mStages[0] = { "dof/image.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "dof/horizontalDof.frag", NULL, 0, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderHorizontalDof);

		shaderDesc.mStages[0] = { "dof/image.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "dof/composite.frag", NULL, 0, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pShaderComposite);
	}

	void RemoveShaders()
	{
		removeShader(pRenderer, pShaderBasic);
		removeShader(pRenderer, pShaderLight);
		removeShader(pRenderer, pShaderGenCoc);
		removeShader(pRenderer, pShaderDownres);
		removeShader(pRenderer, pShaderMaxFilterNearCoCX);
		removeShader(pRenderer, pShaderMaxFilterNearCoCY);
		removeShader(pRenderer, pShaderBoxFilterNearCoCX);
		removeShader(pRenderer, pShaderBoxFilterNearCoCY);
		removeShader(pRenderer, pShaderHorizontalDof);
		removeShader(pRenderer, pShaderComposite);
	}

	// Descriptor Sets

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
				#ifndef TARGET_IOS
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
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
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

		// Downres
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderDownres };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureDownres);
			}

			DescriptorSetDesc setDesc = { pRootSignatureDownres, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsDownres[0]);
			setDesc = { pRootSignatureDownres, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsDownres[1]);
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

		// Far Horizontal DoF
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderHorizontalDof };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureHorizontalPass);
			}

			DescriptorSetDesc setDesc = { pRootSignatureHorizontalPass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsHorizontalPass[0]);
			setDesc = { pRootSignatureHorizontalPass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsHorizontalPass[1]);
		}

		// Composite
		{
			const char* pStaticSamplerNames [] = { "samplerLinear", "samplerPoint" };
			Sampler* pStaticSamplers [] = { pSamplerLinearClampEdge, pSamplerPoint };
			uint        numStaticSamplers = 2;
			{
				Shader* shaders[1] = { pShaderComposite };
				RootSignatureDesc rootDesc = {};
				rootDesc.mShaderCount = 1;
				rootDesc.ppShaders = shaders;
				rootDesc.mStaticSamplerCount = numStaticSamplers;
				rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
				rootDesc.ppStaticSamplers = pStaticSamplers;
				addRootSignature(pRenderer, &rootDesc, &pRootSignatureCompositePass);
			}

			DescriptorSetDesc setDesc = { pRootSignatureCompositePass, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsCompositePass[0]);
			setDesc = { pRootSignatureCompositePass, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
			addDescriptorSet(pRenderer, &setDesc, &pDescriptorSetsCompositePass[1]);
		}
	}

	void RemoveDescriptorSets()
	{
		removeRootSignature(pRenderer, pRootSignatureScene);
		removeRootSignature(pRenderer, pRootSignatureCoC);
		removeRootSignature(pRenderer, pRootSignatureDownres);
		removeRootSignature(pRenderer, pRootSignatureMaxFilterNearX);
		removeRootSignature(pRenderer, pRootSignatureMaxFilterNearY);
		removeRootSignature(pRenderer, pRootSignatureBoxFilterNearX);
		removeRootSignature(pRenderer, pRootSignatureBoxFilterNearY);
		removeRootSignature(pRenderer, pRootSignatureHorizontalPass);
		removeRootSignature(pRenderer, pRootSignatureCompositePass);

		for (int i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
		{
			if (pDescriptorSetsScene[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsScene[i]);
			if (pDescriptorSetsCoc[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsCoc[i]);
			if (pDescriptorSetsDownres[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsDownres[i]);
			if (pDescriptorSetsNearMaxFilterX[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearMaxFilterX[i]);
			if (pDescriptorSetsNearMaxFilterY[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearMaxFilterY[i]);
			if (pDescriptorSetsNearBoxFilterX[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearBoxFilterX[i]);
			if (pDescriptorSetsNearBoxFilterY[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsNearBoxFilterY[i]);
			if (pDescriptorSetsHorizontalPass[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsHorizontalPass[i]);
			if (pDescriptorSetsCompositePass[i])
				removeDescriptorSet(pRenderer, pDescriptorSetsCompositePass[i]);
		}
	}

	void PrepareDescriptorSets()
	{
		// HDR Scene 
		{
			DescriptorData params[2] = {};
			params[0].pName = "cbPerProp";
			params[0].ppBuffers = &gSponzaSceneData.pConstantBuffer;
			params[1].pName = "textureMaps";
			params[1].ppTextures = pMaterialTexturesSponza;
			params[1].mCount = TOTAL_SPONZA_IMGS;
			updateDescriptorSet(pRenderer, 0, pDescriptorSetsScene[0], 2, params);

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
					pDescriptorSetsDownres[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 2,
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

		// 2-Component for Far Field and 1-Component for Near Field
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[4] = {};
				params[0].pName = "TextureColor";
				params[0].ppTextures = &pRenderTargetColorDownres[i]->pTexture;
				params[1].pName = "TextureCoC";
				params[1].ppTextures = &pRenderTargetCoCDowres[i]->pTexture;
				params[2].pName = "TextureNearCoC";
				params[2].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				params[3].pName = "UniformDOF";
				params[3].ppBuffers = &pUniformBuffersDOF[i];
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsHorizontalPass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 4,
					params);
			}
		}

		// Composite
		{
			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				DescriptorData params[10] = {};
				params[0].pName = "TextureCoC";
				params[0].ppTextures = &pRenderTargetCoC[i]->pTexture;
				params[1].pName = "TextureColor";
				params[1].ppTextures = &pRenderTargetHDR[i]->pTexture;
				params[2].pName = "UniformDOF";
				params[2].ppBuffers = &pUniformBuffersDOF[i];
				params[3].pName = "TextureFarR";
				params[3].ppTextures = &pRenderTargetFarR[i]->pTexture;
				params[4].pName = "TextureFarG";
				params[4].ppTextures = &pRenderTargetFarG[i]->pTexture;
				params[5].pName = "TextureFarB";
				params[5].ppTextures = &pRenderTargetFarB[i]->pTexture;
				params[6].pName = "TextureNearR";
				params[6].ppTextures = &pRenderTargetNearR[i]->pTexture;
				params[7].pName = "TextureNearG";
				params[7].ppTextures = &pRenderTargetNearG[i]->pTexture;
				params[8].pName = "TextureNearB";
				params[8].ppTextures = &pRenderTargetNearB[i]->pTexture;
				params[9].pName = "TextureNearCoC";
				params[9].ppTextures = &pRenderTargetFilterNearCoCFinal[i]->pTexture;
				updateDescriptorSet(pRenderer, i,
					pDescriptorSetsCompositePass[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 10,
					params);
			}
		}
	}

	// Render Targets

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
			rtDesc.mWidth = mSettings.mWidth / 2;
			rtDesc.mHeight = mSettings.mHeight / 2;
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
			}
		}

		// Weights
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R8_UNORM;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetCoCDowres[0]->mDesc.mWidth;
			rtDesc.mHeight = pRenderTargetCoCDowres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetWeights[i]);
			}
		}

		// Filter NearCoC
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R8_UNORM;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = mSettings.mWidth / 2;
			rtDesc.mHeight = mSettings.mHeight / 2;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFilterNearCoC[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFilterNearCoCFinal[i]);
			}
		}

		// Far, R, G, B Components
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetColorDownres[0]->mDesc.mWidth;
			rtDesc.mHeight =  pRenderTargetColorDownres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFarR[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFarG[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetFarB[i]);
			}
		}

		// Near, R, G, B Components
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue = clearVal;
			rtDesc.mFormat = TinyImageFormat_R16G16_SFLOAT;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = pRenderTargetColorDownres[0]->mDesc.mWidth;
			rtDesc.mHeight =  pRenderTargetColorDownres[0]->mDesc.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;

			for (int i = 0; i < gImageCount; ++i)
			{
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetNearR[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetNearG[i]);
				addRenderTarget(pRenderer, &rtDesc, &pRenderTargetNearB[i]);
			}
		}
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
			removeRenderTarget(pRenderer, pRenderTargetFilterNearCoC[i]);
			removeRenderTarget(pRenderer, pRenderTargetFilterNearCoCFinal[i]);
			removeRenderTarget(pRenderer, pRenderTargetFarR[i]);
			removeRenderTarget(pRenderer, pRenderTargetFarG[i]);
			removeRenderTarget(pRenderer, pRenderTargetFarB[i]);
			removeRenderTarget(pRenderer, pRenderTargetNearR[i]);
			removeRenderTarget(pRenderer, pRenderTargetNearG[i]);
			removeRenderTarget(pRenderer, pRenderTargetNearB[i]);
			removeRenderTarget(pRenderer, pRenderTargetWeights[i]);
		}
	}

	// Pipelines

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
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats = &pRenderTargetCoC[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pRenderTargetCoC[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetCoC[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureCoC;
			pipelineSettings.pShaderProgram = pShaderGenCoc;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineCoC);
		}

		// Downres
		{
			TinyImageFormat formats[2] =
			{
				pRenderTargetCoCDowres[0]->mDesc.mFormat,
				pRenderTargetColorDownres[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 2;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = pRenderTargetCoCDowres[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetCoCDowres[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureDownres;
			pipelineSettings.pShaderProgram = pShaderDownres;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineDownres);
		}

		// Filter Near CoC X
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
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
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
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


		// Horizontal Filter
		{
			TinyImageFormat formats[7] =
			{
				pRenderTargetFarR[0]->mDesc.mFormat,
				pRenderTargetFarG[0]->mDesc.mFormat,
				pRenderTargetFarB[0]->mDesc.mFormat,
				pRenderTargetNearR[0]->mDesc.mFormat,
				pRenderTargetNearG[0]->mDesc.mFormat,
				pRenderTargetNearB[0]->mDesc.mFormat,
				pRenderTargetWeights[0]->mDesc.mFormat,
			};

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 6;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats = formats;
			pipelineSettings.mSampleCount = pRenderTargetFarR[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pRenderTargetFarR[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureHorizontalPass;
			pipelineSettings.pShaderProgram = pShaderHorizontalDof;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineHorizontalDOF);
		}

		// Composite
		{
			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthNone;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats = &pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality = pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignatureCompositePass;
			pipelineSettings.pShaderProgram = pShaderComposite;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineComposite);
		}
	}

	void RemovePipelines()
	{
		removePipeline(pRenderer, pPipelineScene);
		removePipeline(pRenderer, pPipelineLight);
		removePipeline(pRenderer, pPipelineCoC);
		removePipeline(pRenderer, pPipelineDownres);
		removePipeline(pRenderer, pPipelineMaxFilterNearCoCX);
		removePipeline(pRenderer, pPipelineMaxFilterNearCoCY);
		removePipeline(pRenderer, pPipelineBoxFilterNearCoCX);
		removePipeline(pRenderer, pPipelineBoxFilterNearCoCY);
		removePipeline(pRenderer, pPipelineHorizontalDOF);
		removePipeline(pRenderer, pPipelineComposite);
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
		addCmd_n(pCmdPool, false, gImageCount, &ppCmdsHorizontalDof);
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
		removeCmd_n(pCmdPool, gImageCount, ppCmdsHorizontalDof);
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

	//Loads sponza textures and Sponza mesh
	bool LoadSponza()
	{
		//load Sponza
		//eastl::vector<Image> toLoad(TOTAL_IMGS);
		//adding material textures
		for (int i = 0; i < TOTAL_SPONZA_IMGS; ++i)
		{
			TextureLoadDesc textureDesc = {};
			textureDesc.mRoot = FSR_Textures;
			textureDesc.pFilename = pMaterialImageFileNames[i];
			textureDesc.ppTexture = &pMaterialTexturesSponza[i];
			addResource(&textureDesc, true);
		}

		AssimpImporter importer;

		AssimpImporter::Model gModel_Sponza;
		eastl::string sceneFullPath = FileSystem::FixPath(gModel_Sponza_File, FSRoot::FSR_Meshes);
		if (!importer.ImportModel(sceneFullPath.c_str(), &gModel_Sponza))
		{
			LOGF(eERROR, "Failed to load %s", FileSystem::GetFileNameAndExtension(sceneFullPath).c_str());
			finishResourceLoading();
			return false;
		}

		size_t meshCount = gModel_Sponza.mMeshArray.size();
		size_t sponza_matCount = gModel_Sponza.mMaterialList.size();

		gSponzaSceneData.WorldMatrix = mat4::identity();

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

			for (int j = 0; j < pMeshBatch->mCountIndices; j++)
			{
				pMeshBatch->IndicesData.push_back(subMesh.mIndices[j]);
			}

			for (int j = 0; j < pMeshBatch->mCountVertices; j++)
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

		AssignSponzaTextures();

		{
			AssimpImporter::Model sphereModel;
			{
				if (!importer.ImportModel("../../../../art/Meshes/lowpoly/geosphere.obj", &sphereModel))
				{
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

				pMeshBatch->mCountIndices = subMesh.mIndices.size();

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

		for (uint i = 0; i < TOTAL_SPONZA_IMGS; ++i)
			removeResource(pMaterialTexturesSponza[i]);

		removeResource(gLightMesh->pIndicesStream);
		//removeResource(gLightMesh->pNormalStream);
		removeResource(gLightMesh->pPositionStream);
		//removeResource(gLightMesh->pUVStream);
	}
};

DEFINE_APPLICATION_MAIN(CircularDOF)