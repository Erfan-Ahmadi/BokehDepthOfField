#include "../common.h"

#include "Common_3/OS/Interfaces/IProfiler.h"
#include "Middleware_3/UI/AppUI.h"

#include "Common_3/ThirdParty/OpenSource/EASTL/string.h"
#include "Common_3/ThirdParty/OpenSource/EASTL/vector.h"

#include "Common_3/OS/Interfaces/IMemory.h"
#include "Common_3/Tools/AssimpImporter/AssimpImporter.h"

//--------------------------------------------------------------------------------------------
// GLOBAL DEFINTIONS
//--------------------------------------------------------------------------------------------

uint32_t gFrameIndex			= 0;

const uint32_t gImageCount		= 3;

bool bToggleMicroProfiler		= false;
bool bPrevToggleMicroProfiler	= false;

//--------------------------------------------------------------------------------------------
// STRUCT DEFINTIONS
//--------------------------------------------------------------------------------------------

struct MeshBatch
{
	Buffer* pPositionStream;
	Buffer* pNormalStream;
	Buffer* pUVStream;
	Buffer* pIndicesStream;
	uint32_t mCountIndices;
};

struct SceneData
{
	eastl::vector<MeshBatch*> meshes;
};

struct UniformData
{
	mat4 world;
	mat4 view;
	mat4 proj;
};

//--------------------------------------------------------------------------------------------
// RENDERING PIPELINE DATA
//--------------------------------------------------------------------------------------------

Renderer* pRenderer																= NULL;

Queue* pGraphicsQueue															= NULL;

CmdPool* pCmdPool																= NULL;
Cmd** ppCmds 																	= NULL;

SwapChain* pSwapChain 															= NULL;

Fence* pRenderCompleteFences[gImageCount] 										= { NULL };

Semaphore* pImageAcquiredSemaphore 												= NULL;
Semaphore* pRenderCompleteSemaphores[gImageCount] 								= { NULL };

Pipeline* pPipelineForwardPass 													= NULL;

Shader* pBasicShader 															= NULL;

DescriptorSet* pDescriptorSets[DESCRIPTOR_UPDATE_FREQ_COUNT] 					= { NULL };

RootSignature* pRootSignature 													= NULL;

Sampler* pSamplerLinear;

RenderTarget* pDepthBuffer;

RasterizerState* pRasterDefault 												= NULL;

DepthState* pDepthDefault 														= NULL;
DepthState* pDepthNone 															= NULL;

Buffer* pUniformBuffers[gImageCount] 											= { NULL };

UniformData gUniformData;
SceneData gSceneData;

//--------------------------------------------------------------------------------------------
// THE FORGE OBJECTS
//--------------------------------------------------------------------------------------------

UIApp gAppUI;
GuiComponent* pGui						= NULL;
VirtualJoystickUI gVirtualJoystick;
GpuProfiler* pGpuProfiler				= NULL;
ICameraController* pCameraController	= NULL;
TextDrawDesc gFrameTimeDraw				= TextDrawDesc(0, 0xff00ffff, 18);


const char* pszBases[FSR_Count] = {
	"../../../../src/Shaders/bin",													// FSR_BinShaders
	"../../../../src/CircularDOF/",														// FSR_SrcShaders
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


class Example: public IApp
{
	public:
	Example() {}

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

		bool succeed = LoadModels();

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

		// Camera
		CameraMotionParameters cmp { 40.0f, 30.0f, 100.0f };
		vec3                   camPos { 0.0f, 0.0f, 0.8f };
		vec3                   lookAt { 0.0f, 0.0f, 5.0f };

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
		typedef bool (*CameraInputHandler)(InputActionContext * ctx, uint32_t index);
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

		// Exit profile
		exitProfiler();

		UnloadModels();

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

		if (!gAppUI.Load(pSwapChain->ppSwapchainRenderTargets)) return false;

		if (!gVirtualJoystick.Load(pSwapChain->ppSwapchainRenderTargets[0]))
			return false;

		loadProfiler(&gAppUI, mSettings.mWidth, mSettings.mHeight);

		AddPipelines();

		PrepareDescriptorSets();

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

		// update camera with time
		mat4 viewMat = pCameraController->getViewMatrix();

		const float aspectInverse =
			(float)mSettings.mHeight / (float)mSettings.mWidth;
		const float horizontal_fov = PI / 2.0f;
		mat4 projMat =
			mat4::perspective(horizontal_fov, aspectInverse, 0.1f, 100.0f);

		gUniformData.view = viewMat;
		gUniformData.proj = projMat;

		// Update Instance Data
		gUniformData.world = mat4::translation(Vector3(0.0f, -1.5f, 7)) *
			mat4::rotationY(currentTime) *
			mat4::scale(Vector3(0.4f));

		viewMat.setTranslation(vec3(0));

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
		acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL,
			&gFrameIndex);

		Semaphore* pRenderCompleteSemaphore =
			pRenderCompleteSemaphores[gFrameIndex];
		Fence* pRenderCompleteFence = pRenderCompleteFences[gFrameIndex];

		// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
		FenceStatus fenceStatus;
		getFenceStatus(pRenderer, pRenderCompleteFence, &fenceStatus);
		if (fenceStatus == FENCE_STATUS_INCOMPLETE)
			waitForFences(pRenderer, 1, &pRenderCompleteFence);

		// Update uniform buffers
		BufferUpdateDesc viewProjCbv = { pUniformBuffers[gFrameIndex],
										&gUniformData };
		updateResource(&viewProjCbv);

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

		// Forward Pass
		Cmd* cmd = ppCmds[gFrameIndex];
		{
			RenderTarget* pSwapChainRenderTarget =
				pSwapChain->ppSwapchainRenderTargets[gFrameIndex];

			beginCmd(cmd);
			cmdBeginGpuFrameProfile(cmd, pGpuProfiler);
			{
				cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Forward Pass", true);

				TextureBarrier textureBarriers[2] = {
					{pSwapChainRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET},
					{pDepthBuffer->pTexture, RESOURCE_STATE_DEPTH_WRITE} };

				cmdResourceBarrier(cmd, 0, nullptr, 2, textureBarriers);

				cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, pDepthBuffer,
					&loadActions, NULL, NULL, -1, -1);
				cmdSetViewport(
					cmd, 0.0f, 0.0f, (float)pSwapChainRenderTarget->mDesc.mWidth,
					(float)pSwapChainRenderTarget->mDesc.mHeight, 0.0f, 1.0f);
				cmdSetScissor(cmd, 0, 0, pSwapChainRenderTarget->mDesc.mWidth,
					pSwapChainRenderTarget->mDesc.mHeight);

				cmdBindDescriptorSet(cmd, gFrameIndex,
					pDescriptorSets[DESCRIPTOR_UPDATE_FREQ_PER_FRAME]);

				cmdBindPipeline(cmd, pPipelineForwardPass);
				{
					for (int i = 0; i < gSceneData.meshes.size(); ++i)
					{
						Buffer* pVertexBuffers [] = { gSceneData.meshes[i]->pPositionStream,
													gSceneData.meshes[i]->pNormalStream };
						cmdBindVertexBuffer(cmd, 2, pVertexBuffers, NULL);
						cmdBindIndexBuffer(cmd, gSceneData.meshes[i]->pIndicesStream, 0);
						cmdDrawIndexed(cmd, gSceneData.meshes[i]->mCountIndices, 0, 0);
					}
				}

				cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

				loadActions = {};
				loadActions.mLoadActionsColor[0] = LOAD_ACTION_LOAD;
				cmdBindRenderTargets(cmd, 1, &pSwapChainRenderTarget, NULL,
					&loadActions, NULL, NULL, -1, -1);
				cmdBeginGpuTimestampQuery(cmd, pGpuProfiler, "Draw UI", true);
				static HiresTimer gTimer;
				gTimer.GetUSec(true);

				gVirtualJoystick.Draw(cmd, { 1.0f, 1.0f, 1.0f, 1.0f });

				gAppUI.DrawText(
					cmd, float2(8, 15),
					eastl::string()
					.sprintf("CPU %f ms", gTimer.GetUSecAverage() / 1000.0f)
					.c_str(),
					&gFrameTimeDraw);

				#if !defined(__ANDROID__)
				gAppUI.DrawText(
					cmd, float2(8, 40),
					eastl::string()
					.sprintf("GPU %f ms",
					(float)pGpuProfiler->mCumulativeTime * 1000.0f)
					.c_str(),
					&gFrameTimeDraw);
				gAppUI.DrawDebugGpuProfile(cmd, float2(8, 65), pGpuProfiler, NULL);
				#endif

				cmdDrawProfiler();

				gAppUI.Gui(pGui);

				gAppUI.Draw(cmd);
				cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
				cmdEndGpuTimestampQuery(cmd, pGpuProfiler);

				textureBarriers[0] = { pSwapChainRenderTarget->pTexture,
									  RESOURCE_STATE_PRESENT };
				cmdResourceBarrier(cmd, 0, NULL, 1, textureBarriers);
			}
			cmdEndGpuFrameProfile(cmd, pGpuProfiler);
			endCmd(cmd);
		}

		// Submit Second Pass Command Buffer
		queueSubmit(pGraphicsQueue, 1, &cmd, pRenderCompleteFence, 1,
			&pImageAcquiredSemaphore, 1, &pRenderCompleteSemaphore);

		queuePresent(pGraphicsQueue, pSwapChain, gFrameIndex, 1,
			&pRenderCompleteSemaphore);

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
		swapChainDesc.mEnableVsync = true;
		swapChainDesc.mWidth = mSettings.mWidth;
		swapChainDesc.mHeight = mSettings.mHeight;
		swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true);
		::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
		return pSwapChain != NULL;
	}

	const char* GetName() { return "Example"; }

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
								   MIPMAP_MODE_NEAREST,
								   ADDRESS_MODE_CLAMP_TO_EDGE,
								   ADDRESS_MODE_CLAMP_TO_EDGE,
								   ADDRESS_MODE_CLAMP_TO_EDGE };
		addSampler(pRenderer, &samplerDesc, &pSamplerLinear);
	}

	void RemoveSamplers() { removeSampler(pRenderer, pSamplerLinear); }

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
		// Forward
		{
			BufferLoadDesc bufferDesc = {};
			bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
			bufferDesc.mDesc.mSize = sizeof(gUniformData);
			bufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
			bufferDesc.pData = NULL;

			for (uint32_t i = 0; i < gImageCount; ++i)
			{
				bufferDesc.ppBuffer = &pUniformBuffers[i];
				addResource(&bufferDesc);
			}
		}
	}

	void RemoveBuffers()
	{
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			removeResource(pUniformBuffers[i]);
		}
	}

	// Shaders

	void AddShaders()
	{
		// Basic Shader
		ShaderLoadDesc shaderDesc = {};
		shaderDesc.mStages[0] = { "basic.vert", NULL, 0, FSR_SrcShaders };
		shaderDesc.mStages[1] = { "basic.frag", NULL, 0, FSR_SrcShaders };
		addShader(pRenderer, &shaderDesc, &pBasicShader);
	}

	void RemoveShaders()
	{
		removeShader(pRenderer, pBasicShader);
	}

	// Descriptor Sets

	void AddDescriptorSets()
	{
		// Resource Binding
		const char* samplerNames = { "uSampler0 " };
		// Root Signature for Forward Pipeline
		{
			Shader* shaders[1] = { pBasicShader };
			RootSignatureDesc rootDesc = {};
			rootDesc.mShaderCount = 1;
			rootDesc.ppShaders = shaders;
			rootDesc.mStaticSamplerCount = 1;
			rootDesc.ppStaticSamplerNames = &samplerNames;
			rootDesc.ppStaticSamplers = &pSamplerLinear;
			addRootSignature(pRenderer, &rootDesc, &pRootSignature);
		}

		DescriptorSetDesc setDesc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE,
									 1 };
		addDescriptorSet(pRenderer, &setDesc, &pDescriptorSets[0]);
		setDesc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount };
		addDescriptorSet(pRenderer, &setDesc, &pDescriptorSets[1]);
	}

	void RemoveDescriptorSets()
	{
		removeRootSignature(pRenderer, pRootSignature);
		for (int i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
		{
			if (pDescriptorSets[i])
				removeDescriptorSet(pRenderer, pDescriptorSets[i]);
		}
	}

	void PrepareDescriptorSets()
	{
		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			DescriptorData params[1] = {};
			params[0].pName = "UniformData";
			params[0].ppBuffers = &pUniformBuffers[i];
			updateDescriptorSet(pRenderer, i,
				pDescriptorSets[DESCRIPTOR_UPDATE_FREQ_PER_FRAME], 1,
				params);
		}
	}

	// Pipelines

	void AddPipelines()
	{
		{
			VertexLayout vertexLayout = {};
			vertexLayout.mAttribCount = 2;

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

			PipelineDesc desc = {};
			desc.mType = PIPELINE_TYPE_GRAPHICS;
			GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
			pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			pipelineSettings.mRenderTargetCount = 1;
			pipelineSettings.pDepthState = pDepthDefault;
			pipelineSettings.mDepthStencilFormat = pDepthBuffer->mDesc.mFormat;
			pipelineSettings.pColorFormats =
				&pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mFormat;
			pipelineSettings.mSampleCount =
				pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleCount;
			pipelineSettings.mSampleQuality =
				pSwapChain->ppSwapchainRenderTargets[0]->mDesc.mSampleQuality;
			pipelineSettings.pRootSignature = pRootSignature;
			pipelineSettings.pShaderProgram = pBasicShader;
			pipelineSettings.pVertexLayout = &vertexLayout;
			pipelineSettings.pRasterizerState = pRasterDefault;

			addPipeline(pRenderer, &desc, &pPipelineForwardPass);
		}
	}

	void RemovePipelines()
	{
		removePipeline(pRenderer, pPipelineForwardPass);
	}

	// Render Targets

	void AddRenderTargets()
	{
		ClearValue colorClearBlack = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Depth Buffer
		{
			RenderTargetDesc rtDesc = {};
			rtDesc.mArraySize = 1;
			rtDesc.mClearValue.depth = 1.0f;
			rtDesc.mClearValue.stencil = 0;
			rtDesc.mFormat = TinyImageFormat_D24_UNORM_S8_UINT;
			rtDesc.mDepth = 1;
			rtDesc.mWidth = mSettings.mWidth;
			rtDesc.mHeight = mSettings.mHeight;
			rtDesc.mSampleCount = SAMPLE_COUNT_1;
			rtDesc.mSampleQuality = 0;
			::addRenderTarget(pRenderer, &rtDesc, &pDepthBuffer);
		}
	}

	void RemoveRenderTargets()
	{
		removeRenderTarget(pRenderer, pDepthBuffer);
	}

	// Commands

	void AddCmds()
	{
		addCmdPool(pRenderer, pGraphicsQueue, false, &pCmdPool);
		addCmd_n(pCmdPool, false, gImageCount, &ppCmds);
	}

	void RemoveCmds()
	{
		removeCmd_n(pCmdPool, gImageCount, ppCmds);
		removeCmdPool(pRenderer, pCmdPool);
	}

	// Models

	bool LoadModels()
	{
		AssimpImporter importer;

		AssimpImporter::Model model;
		if (!importer.ImportModel("../../../../art/Meshes/chinesedragon.dae",
			&model))
		{
			return false;
		}

		size_t meshSize = model.mMeshArray.size();

		for (size_t i = 0; i < meshSize; ++i)
		{
			AssimpImporter::Mesh subMesh = model.mMeshArray[i];

			MeshBatch* pMeshBatch = (MeshBatch*)conf_placement_new<MeshBatch>(
				conf_calloc(1, sizeof(MeshBatch)));

			gSceneData.meshes.push_back(pMeshBatch);

			// Vertex Buffer
			BufferLoadDesc bufferDesc = {};
			bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
			bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
			bufferDesc.mDesc.mVertexStride = sizeof(float3);
			bufferDesc.mDesc.mSize =
				bufferDesc.mDesc.mVertexStride * subMesh.mPositions.size();
			bufferDesc.pData = subMesh.mPositions.data();
			bufferDesc.ppBuffer = &pMeshBatch->pPositionStream;
			addResource(&bufferDesc);

			bufferDesc.mDesc.mVertexStride = sizeof(float3);
			bufferDesc.mDesc.mSize =
				subMesh.mNormals.size() * bufferDesc.mDesc.mVertexStride;
			bufferDesc.pData = subMesh.mNormals.data();
			bufferDesc.ppBuffer = &pMeshBatch->pNormalStream;
			addResource(&bufferDesc);

			if (subMesh.mUvs.data())
			{
				bufferDesc.mDesc.mVertexStride = sizeof(float2);
				bufferDesc.mDesc.mSize =
					subMesh.mUvs.size() * bufferDesc.mDesc.mVertexStride;
				bufferDesc.pData = subMesh.mUvs.data();
				bufferDesc.ppBuffer = &pMeshBatch->pUVStream;
				addResource(&bufferDesc);
			}

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

		return true;
	}

	void UnloadModels()
	{
		for (size_t i = 0; i < gSceneData.meshes.size(); ++i)
		{
			removeResource(gSceneData.meshes[i]->pPositionStream);
			removeResource(gSceneData.meshes[i]->pNormalStream);
			removeResource(gSceneData.meshes[i]->pIndicesStream);
			if (gSceneData.meshes[i]->pUVStream)
				removeResource(gSceneData.meshes[i]->pUVStream);
		}
	}
};

DEFINE_APPLICATION_MAIN(Example)