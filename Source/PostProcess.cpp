/*******************************************
	PostProcess.cpp

	Main scene and game functions
********************************************/

#include <Windows.h>
#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "CParseLevel.h"
#include "PostProcess.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx10.h"

namespace gen
{

	//*****************************************************************************
	// Post-process data
	//*****************************************************************************

	// Enumeration of different post-processes
	enum PostProcesses
	{
		Copy, Tint, Tint2, GreyNoise, Burn, Distort, Spiral, Water, Retro, Grayscale, Invert,
		NumPostProcesses
	};

	// Currently used post process
	vector<PostProcesses> CurrentPostProcessList = { Copy, Invert };


	// Separate effect file for post-processes, not necessary to use a separate file, but convenient given the architecture of this lab
	ID3D10Effect* PPEffect;

	// Technique name for each post-process
	const string PPTechniqueNames[NumPostProcesses] = { "PPCopy", "PPTint", "PPTint2", "PPGreyNoise", "PPBurn", "PPDistort", "PPSpiral", "PPWater", "PPRetro", "PPGrayscale", "PPInvert" };

	// Technique pointers for each post-process
	ID3D10EffectTechnique* PPTechniques[NumPostProcesses];


	// Will render the scene to a texture in a first pass, then copy that texture to the back buffer in a second post-processing pass
	// So need a texture and two "views": a render target view (to render into the texture - 1st pass) and a shader resource view (use the rendered texture as a normal texture - 2nd pass)
	ID3D10Texture2D*          SceneTexture = NULL;
	ID3D10Texture2D*          SceneTexture2 = NULL;
	ID3D10RenderTargetView*   SceneRenderTarget = NULL;
	ID3D10RenderTargetView*   SceneRenderTarget2 = NULL;
	ID3D10ShaderResourceView* SceneShaderResource = NULL;
	ID3D10ShaderResourceView* SceneShaderResource2 = NULL;

	// Additional textures used by post-processes
	ID3D10ShaderResourceView* NoiseMap = NULL;
	ID3D10ShaderResourceView* BurnMap = NULL;
	ID3D10ShaderResourceView* DistortMap = NULL;

	// Variables to link C++ post-process textures to HLSL shader variables
	ID3D10EffectShaderResourceVariable* SceneTextureVar = NULL;
	ID3D10EffectShaderResourceVariable* SceneTexture2Var = NULL;
	ID3D10EffectShaderResourceVariable* PostProcessMapVar = NULL; // Single shader variable used for the three maps above (noise, burn, distort). Only one is needed at a time

	// Other variables for individual post-processes
	ID3D10EffectVectorVariable* TintColourVar = NULL;
	ID3D10EffectVectorVariable* TintColour2Var = NULL;
	ID3D10EffectVectorVariable* NoiseScaleVar = NULL;
	ID3D10EffectVectorVariable* NoiseOffsetVar = NULL;
	ID3D10EffectScalarVariable* DistortLevelVar = NULL;
	ID3D10EffectScalarVariable* BurnLevelVar = NULL;
	ID3D10EffectScalarVariable* WiggleVar = NULL;
	ID3D10EffectScalarVariable* BlurLevelVar = NULL;

	// retro settings
	ID3D10EffectScalarVariable* PixelationVar = NULL;
	ID3D10EffectScalarVariable* ColourPalletVar = NULL;

	// Dimensions of the viewport
	ID3D10EffectScalarVariable* ViewportWidthVar = NULL;
	ID3D10EffectScalarVariable* ViewportHeightVar = NULL;


	//*****************************************************************************


	//-----------------------------------------------------------------------------
	// Constants
	//-----------------------------------------------------------------------------

	// Control speed
	const float CameraRotSpeed = 2.0f;
	float CameraMoveSpeed = 80.0f;

	// Amount of time to pass before calculating new average update time
	const float UpdateTimePeriod = 0.25f;

	// motion blue
	bool motionBlurEnabled = false;
	float motionBlurAmount = 0.2f;

	// Demo state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//-----------------------------------------------------------------------------
	// Global system variables
	//-----------------------------------------------------------------------------

	// Folders used for meshes/textures and effect files
	extern const string MediaFolder;
	extern const string ShaderFolder;

	// Get reference to global DirectX variables from another source file
	extern ID3D10Device*           g_pd3dDevice;
	extern IDXGISwapChain*         SwapChain;
	extern ID3D10DepthStencilView* DepthStencilView;
	extern ID3D10RenderTargetView* BackBufferRenderTarget;
	extern ID3DX10Font*            OSDFont;

	// Actual viewport dimensions (fullscreen or windowed)
	extern TUInt32 BackBufferWidth;
	extern TUInt32 BackBufferHeight;

	// Current mouse position
	extern TUInt32 MouseX;
	extern TUInt32 MouseY;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;


	//-----------------------------------------------------------------------------
	// Global game/scene variables
	//-----------------------------------------------------------------------------

	// Entity manager and level parser
	CEntityManager EntityManager;
	CParseLevel LevelParser(&EntityManager);
	bool firstSceneRenderer = false;

	// Other scene elements
	const int NumLights = 2;
	CLight*  Lights[NumLights];
	CCamera* MainCamera;

	// Sum of recent update times and number of times in the sum - used to calculate
	// average over a given time period
	float SumUpdateTimes = 0.0f;
	int NumUpdateTimes = 0;
	float AverageUpdateTime = -1.0f; // Invalid value at first


	//-----------------------------------------------------------------------------
	// Game Constants
	//-----------------------------------------------------------------------------

	// Lighting
	const SColourRGBA AmbientColour(0.3f, 0.3f, 0.4f, 1.0f);
	CVector3 LightCentre(0.0f, 30.0f, 50.0f);
	const float LightOrbit = 170.0f;
	const float LightOrbitSpeed = 0.2f;


	//-----------------------------------------------------------------------------
	// Scene management
	//-----------------------------------------------------------------------------

	// Creates the scene geometry
	bool SceneSetup()
	{
		// Prepare render methods
		InitialiseMethods();

		// Read templates and entities from XML file
		if (!LevelParser.ParseFile("Entities.xml")) return false;

		// Set camera position and clip planes suitable for space game
		MainCamera = new CCamera(CVector3(0.0f, 50, -150), CVector3(ToRadians(15.0f), 0, 0));
		MainCamera->SetNearFarClip(2.0f, 300000.0f);

		// Sunlight
		Lights[0] = new CLight(CVector3(-10000.0f, 6000.0f, 0000.0f), SColourRGBA(1.0f, 0.8f, 0.6f) * 12000, 20000.0f); // Colour is multiplied by light brightness

		// Light orbiting area
		Lights[1] = new CLight(LightCentre, SColourRGBA(0.0f, 0.2f, 1.0f) * 50, 100.0f);

		return true;
	}


	// Release everything in the scene
	void SceneShutdown()
	{
		// Release render methods
		ReleaseMethods();

		// Release lights
		for (int light = NumLights - 1; light >= 0; --light)
		{
			delete Lights[light];
		}

		// Release camera
		delete MainCamera;

		// Destroy all entities
		EntityManager.DestroyAllEntities();
		EntityManager.DestroyAllTemplates();
	}


	//*****************************************************************************
	// Post Processing Setup
	//*****************************************************************************

	// Prepare resources required for the post-processing pass
	bool PostProcessSetup()
	{
		// Create the "scene texture" - the texture into which the scene will be rendered in the first pass
		D3D10_TEXTURE2D_DESC textureDesc;
		textureDesc.Width = BackBufferWidth;  // Match views to viewport size
		textureDesc.Height = BackBufferHeight;
		textureDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we will have to render every level)
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D10_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and pass it to shaders
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;
		if (FAILED(g_pd3dDevice->CreateTexture2D(&textureDesc, NULL, &SceneTexture))) return false;
		if (FAILED(g_pd3dDevice->CreateTexture2D(&textureDesc, NULL, &SceneTexture2))) return false;

		// Get a "view" of the texture as a render target - giving us an interface for rendering to the texture
		if (FAILED(g_pd3dDevice->CreateRenderTargetView(SceneTexture, NULL, &SceneRenderTarget))) return false;
		if (FAILED(g_pd3dDevice->CreateRenderTargetView(SceneTexture2, NULL, &SceneRenderTarget2))) return false;

		// And get a shader-resource "view" - giving us an interface for passing the texture to shaders
		D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
		srDesc.Format = textureDesc.Format;
		srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
		srDesc.Texture2D.MostDetailedMip = 0;
		srDesc.Texture2D.MipLevels = 1;
		if (FAILED(g_pd3dDevice->CreateShaderResourceView(SceneTexture, &srDesc, &SceneShaderResource))) return false;
		if (FAILED(g_pd3dDevice->CreateShaderResourceView(SceneTexture2, &srDesc, &SceneShaderResource2))) return false;

		// Load post-processing support textures
		if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, (MediaFolder + "Noise.png").c_str(), NULL, NULL, &NoiseMap, NULL))) return false;
		if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, (MediaFolder + "Burn.png").c_str(), NULL, NULL, &BurnMap, NULL))) return false;
		if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, (MediaFolder + "Distort.png").c_str(), NULL, NULL, &DistortMap, NULL))) return false;


		// Load and compile a separate effect file for post-processes.
		ID3D10Blob* pErrors;
		DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // These "flags" are used to set the compiler options

		string fullFileName = ShaderFolder + "PostProcess.fx";
		if (FAILED(D3DX10CreateEffectFromFile(fullFileName.c_str(), NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &PPEffect, &pErrors, NULL)))
		{
			if (pErrors != 0)  MessageBox(NULL, reinterpret_cast<char*>(pErrors->GetBufferPointer()), "Error", MB_OK); // Compiler error: display error message
			else               MessageBox(NULL, "Error loading FX file. Ensure your FX file is in the same folder as this executable.", "Error", MB_OK);  // No error message - probably file not found
			return false;
		}

		// There's an array of post-processing technique names above - get array of post-process techniques matching those names from the compiled effect file
		for (int pp = 0; pp < NumPostProcesses; pp++)
		{
			PPTechniques[pp] = PPEffect->GetTechniqueByName(PPTechniqueNames[pp].c_str());
		}

		// Link to HLSL variables in post-process shaders
		SceneTextureVar = PPEffect->GetVariableByName("SceneTexture")->AsShaderResource();
		SceneTexture2Var = PPEffect->GetVariableByName("SceneTexture2")->AsShaderResource();
		PostProcessMapVar = PPEffect->GetVariableByName("PostProcessMap")->AsShaderResource();
		TintColourVar = PPEffect->GetVariableByName("TintColour")->AsVector();
		TintColour2Var = PPEffect->GetVariableByName("TintColour2")->AsVector();
		NoiseScaleVar = PPEffect->GetVariableByName("NoiseScale")->AsVector();
		NoiseOffsetVar = PPEffect->GetVariableByName("NoiseOffset")->AsVector();
		DistortLevelVar = PPEffect->GetVariableByName("DistortLevel")->AsScalar();
		BurnLevelVar = PPEffect->GetVariableByName("BurnLevel")->AsScalar();
		WiggleVar = PPEffect->GetVariableByName("Wiggle")->AsScalar();
		BlurLevelVar = PPEffect->GetVariableByName("BlurLevel")->AsScalar();

		// Retro
		PixelationVar = PPEffect->GetVariableByName("Pixelation")->AsScalar();
		ColourPalletVar = PPEffect->GetVariableByName("ColourPallet")->AsScalar();

		// Viewport dimensions
		ViewportWidthVar = PPEffect->GetVariableByName("ViewportWidth")->AsScalar();
		ViewportHeightVar = PPEffect->GetVariableByName("ViewportHeight")->AsScalar();

		return true;
	}

	void PostProcessShutdown()
	{
		if (PPEffect)            PPEffect->Release();
		if (DistortMap)          DistortMap->Release();
		if (BurnMap)             BurnMap->Release();
		if (NoiseMap)            NoiseMap->Release();
		if (SceneShaderResource) SceneShaderResource->Release();
		if (SceneRenderTarget)   SceneRenderTarget->Release();
		if (SceneTexture)        SceneTexture->Release();
		if (SceneShaderResource2) SceneShaderResource2->Release();
		if (SceneRenderTarget2)  SceneRenderTarget2->Release();
		if (SceneTexture2)       SceneTexture2->Release();
	}

	//*****************************************************************************


	//-----------------------------------------------------------------------------
	// Game loop functions
	//-----------------------------------------------------------------------------

	// Draw one frame of the scene
	void RenderScene(float updateTime)
	{
		// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
		D3D10_VIEWPORT vp;
		vp.Width = BackBufferWidth;
		vp.Height = BackBufferHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		g_pd3dDevice->RSSetViewports(1, &vp);

		ViewportWidthVar->SetFloat(static_cast<float>(BackBufferWidth));
		ViewportHeightVar->SetFloat(static_cast<float>(BackBufferHeight));


		//************************************************
		// FIRST RENDER PASS - Render scene to texture

		// Specify that the scene texture will be the render target in this first pass (rather than the backbuffer), will share the depth/stencil buffer with the backbuffer though
		g_pd3dDevice->OMSetRenderTargets(1, &SceneRenderTarget, DepthStencilView);/*MISSING - specify scene texture as render target (variables near top of file)*/
		firstSceneRenderer = true;

		// Clear the texture and the depth buffer
		g_pd3dDevice->ClearRenderTargetView(SceneRenderTarget, &AmbientColour.r);
		g_pd3dDevice->ClearRenderTargetView(SceneRenderTarget2, &AmbientColour.r);
		g_pd3dDevice->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);


		// Prepare camera
		MainCamera->SetAspect(static_cast<TFloat32>(BackBufferWidth) / BackBufferHeight);
		MainCamera->CalculateMatrices();
		MainCamera->CalculateFrustrumPlanes();

		// Set camera and light data in shaders
		SetCamera(MainCamera);
		SetAmbientLight(AmbientColour);
		SetLights(&Lights[0]);

		// Render entities and draw on-screen text
		EntityManager.RenderAllEntities(MainCamera);

		//************************************************


		//************************************************
		// PREPARE GOLBAL POST-PROCESS SETTINGS

		// motion blur
		if (motionBlurEnabled)
			BlurLevelVar->SetFloat(motionBlurAmount);
		else
			BlurLevelVar->SetFloat(1);	// this only use current screen turning motion blue off

		//************************************************


		//************************************************
		// PREPARE INDIVIDUAL POST-PROCESS SETTINGS

		int listCount = CurrentPostProcessList.size();
		for (int i = 0; i < listCount; ++i)
		{
			switch (CurrentPostProcessList[i])
			{
			case Tint:
			{
				// Set the colour used to tint the scene
				D3DXCOLOR TintColour = D3DXCOLOR(1.0f, 0.0f, 0.0f, 0.5f); /*= ? ? FILTER - Make a nice colour*/
				TintColourVar->SetRawValue(&TintColour, 0, 12);
			}
			break;

			case Tint2:
			{
				// Set the colour used to tint the scene
				D3DXCOLOR TintColour = D3DXCOLOR(0.0f, 0.0f, 1.0f, 0.5f); /*= ? ? FILTER - Make a nice colour*/
				D3DXCOLOR TintColour2 = D3DXCOLOR(1.0f, 1.0f, 0.0f, 0.5f); /*= ? ? FILTER - Make a nice colour*/
				TintColourVar->SetRawValue(&TintColour, 0, 12);
				TintColour2Var->SetRawValue(&TintColour2, 0, 12);
			}
			break;

			case GreyNoise:
			{
				const float GrainSize = 140; // Fineness of the noise grain

				// Set shader constants - scale and offset for noise. Scaling adjusts how fine the noise is.
				CVector2 NoiseScale = CVector2(BackBufferWidth / GrainSize, BackBufferHeight / GrainSize);
				NoiseScaleVar->SetRawValue(&NoiseScale, 0, 8);

				// The offset is randomised to give a constantly changing noise effect (like tv static)
				CVector2 RandomUVs = CVector2(Random(-1, 1), Random(-1, 1)) * updateTime;/*FILTER - 2 random UVs please*/
				NoiseOffsetVar->SetRawValue(&RandomUVs, 0, 8);

				// Set noise texture
				PostProcessMapVar->SetResource(NoiseMap);
			}
			break;

			case Burn:
			{
				static float BurnLevel = 0.0f;
				const float BurnSpeed = 0.2f;

				// Set and increase the burn level (cycling back to 0 when it reaches 1.0f)
				BurnLevelVar->SetFloat(BurnLevel);
				BurnLevel = Mod(BurnLevel + BurnSpeed * updateTime, 1.0f);

				// Set burn texture
				PostProcessMapVar->SetResource(BurnMap);
			}
			break;


			case Distort:
			{
				// Set the level of distortion
				const float DistortLevel = 0.03f;
				DistortLevelVar->SetFloat(DistortLevel);

				// Set distort texture
				PostProcessMapVar->SetResource(DistortMap);
			}
			break;


			case Spiral:
			{
				static float Wiggle = 0.0f;
				const float WiggleSpeed = 1.0f;

				// Set and increase the amount of spiral - use a tweaked cos wave to animate
				WiggleVar->SetFloat((1.0f - Cos(Wiggle)) * 4.0f);
				Wiggle += WiggleSpeed * updateTime;
				break;
			}

			case Water:
			{
				// Set the colour used to tint the scene
				D3DXCOLOR TintColour = D3DXCOLOR(0.0f, 1.0f, 1.0f, 0.5f); /*= ? ? FILTER - Make a nice colour*/
				TintColourVar->SetRawValue(&TintColour, 0, 12);

				// wiggle
				static float Wiggle = 0.0f;
				const float WiggleSpeed = 1.0f;

				// Set and increase the amount of spiral - use a tweaked cos wave to animate
				WiggleVar->SetFloat(Wiggle);
				Wiggle += WiggleSpeed * updateTime;
				break;
			}

			case Retro:
			{
				PixelationVar->SetFloat(128.0f);
				ColourPalletVar->SetFloat(4.0f);
			}
			}

			//************************************************


			//************************************************
			// SECOND RENDER PASS - Render full screen quad on the back-buffer mapped with the scene texture, the post-process technique will process the scene pixels in some way during the copy

			// Select the back buffer to use for rendering (will ignore depth-buffer for full-screen quad) and select scene texture for use in shader
			// Not going to clear the back-buffer, we're going to overwrite it all
			if (firstSceneRenderer)
			{
				g_pd3dDevice->OMSetRenderTargets(1, &SceneRenderTarget2, DepthStencilView); /*MISSING, 2nd pass specify back buffer as render target*/
				SceneTextureVar->SetResource(SceneShaderResource);/*MISSING, 2nd pass, will use scene texture in shaders - i.e. as a shader resource, again check available variables*/
				firstSceneRenderer = false;

				g_pd3dDevice->IASetInputLayout(NULL);
				g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				PPTechniques[CurrentPostProcessList[i]]->GetPassByIndex(0)->Apply(0);
				g_pd3dDevice->Draw(4, 0);/*MISSING - Post-process pass renderes a quad*/

				SceneTextureVar->SetResource( 0 );
				PPTechniques[CurrentPostProcessList[i]]->GetPassByIndex(0)->Apply(0);
			}
			else
			{
				g_pd3dDevice->OMSetRenderTargets(1, &SceneRenderTarget, DepthStencilView); /*MISSING, 2nd pass specify back buffer as render target*/
				SceneTextureVar->SetResource(SceneShaderResource2);/*MISSING, 2nd pass, will use scene texture in shaders - i.e. as a shader resource, again check available variables*/
				firstSceneRenderer = true;

				g_pd3dDevice->IASetInputLayout(NULL);
				g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				PPTechniques[CurrentPostProcessList[i]]->GetPassByIndex(0)->Apply(0);
				g_pd3dDevice->Draw(4, 0);/*MISSING - Post-process pass renderes a quad*/

				SceneTextureVar->SetResource(0);
				PPTechniques[CurrentPostProcessList[i]]->GetPassByIndex(0)->Apply(0);
			}
		}
		
		g_pd3dDevice->OMSetRenderTargets(1, &BackBufferRenderTarget, DepthStencilView); /*MISSING, 2nd pass specify back buffer as render target*/
		if (firstSceneRenderer)
		{
			SceneTextureVar->SetResource(SceneShaderResource);/*MISSING, 2nd pass, will use scene texture in shaders - i.e. as a shader resource, again check available variables*/
			firstSceneRenderer = false;
		}
		else
		{
			SceneTextureVar->SetResource(SceneShaderResource2);/*MISSING, 2nd pass, will use scene texture in shaders - i.e. as a shader resource, again check available variables*/
			firstSceneRenderer = true;
		}

		// Using special vertex shader than creates its own data for a full screen quad (see .fx file). No need to set vertex/index buffer, just draw 4 vertices of quad
		// Select technique to match currently selected post-process
		PPTechniques[Copy]->GetPassByIndex(0)->Apply(0);
		g_pd3dDevice->Draw(4, 0);/*MISSING - Post-process pass renderes a quad*/

		SceneTexture2Var->SetResource(0);
		PPTechniques[Copy]->GetPassByIndex(0)->Apply(0);

		// Render UI elements last - don't want them post-processed
		RenderImGui();
		RenderSceneText(updateTime);

		// Present the backbuffer contents to the display
		SwapChain->Present(0, 0);
	}

	// render the imgui windows
	void RenderImGui()
	{
		// Start the Dear ImGui frame
		ImGui_ImplDX10_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// test

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// render windows
		ImGui::Render();
		ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
	}

	// Render a single text string at the given position in the given colour, may optionally centre it
	void RenderText(const string& text, int X, int Y, float r, float g, float b, bool centre = false)
	{
		RECT rect;
		if (!centre)
		{
			SetRect(&rect, X, Y, 0, 0);
			OSDFont->DrawText(NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR(r, g, b, 1.0f));
		}
		else
		{
			SetRect(&rect, X - 100, Y, X + 100, 0);
			OSDFont->DrawText(NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR(r, g, b, 1.0f));
		}
	}

	// Render on-screen text each frame
	void RenderSceneText(float updateTime)
	{
		// Accumulate update times to calculate the average over a given period
		SumUpdateTimes += updateTime;
		++NumUpdateTimes;
		if (SumUpdateTimes >= UpdateTimePeriod)
		{
			AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
			SumUpdateTimes = 0.0f;
			NumUpdateTimes = 0;
		}

		// Write FPS text string
		stringstream outText;
		if (AverageUpdateTime >= 0.0f)
		{
			outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
			RenderText(outText.str(), 2, 2, 0.0f, 0.0f, 0.0f);
			RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
			outText.str("");
		}

		//// Output post-process name
		//outText << "Post-Process: ";
		//switch (CurrentPostProcess)
		//{
		//case Copy:
		//	outText << "Copy";
		//	break;
		//case Tint:
		//	outText << "Tint";
		//	break;
		//case Tint2:
		//	outText << "Tint 2 Colours";
		//	break;
		//case GreyNoise:
		//	outText << "Grey Noise";
		//	break;
		//case Burn:
		//	outText << "Burn";
		//	break;
		//case Distort:
		//	outText << "Distort";
		//	break;
		//case Spiral:
		//	outText << "Spiral";
		//	break;
		//case Water:
		//	outText << "Underwater";
		//	break;
		//case Retro:
		//	outText << "Retro";
		//	break;
		//}
		//RenderText(outText.str(), 0, 32, 1.0f, 1.0f, 1.0f);
	}


	// Update the scene between rendering
	void UpdateScene(float updateTime)
	{
		// Call all entity update functions
		EntityManager.UpdateAllEntities(updateTime);

		// Set camera speeds
		// Key F1 used for full screen toggle
		if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
		if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;
		if (KeyHit(Key_F4)) CameraMoveSpeed = 160.0f;
		if (KeyHit(Key_F5)) CameraMoveSpeed = 640.0f;

		// remove post process
		if (KeyHit(Key_Control)) if (CurrentPostProcessList.size() > 1) CurrentPostProcessList.pop_back();

		// Choose post-process
		if (KeyHit(Key_0)) CurrentPostProcessList.push_back(Copy);
		if (KeyHit(Key_1)) CurrentPostProcessList.push_back(Tint2);
		if (KeyHit(Key_2)) motionBlurEnabled = !motionBlurEnabled;
		if (KeyHit(Key_3)) CurrentPostProcessList.push_back(Water);
		if (KeyHit(Key_4)) CurrentPostProcessList.push_back(Retro);

		// others
		if (KeyHit(Key_5)) CurrentPostProcessList.push_back(Distort);
		if (KeyHit(Key_6)) CurrentPostProcessList.push_back(Spiral);
		if (KeyHit(Key_7)) CurrentPostProcessList.push_back(Tint);
		if (KeyHit(Key_8)) CurrentPostProcessList.push_back(GreyNoise);
		if (KeyHit(Key_9)) CurrentPostProcessList.push_back(Burn);

		// Rotate cube and attach light to it
		CEntity* cubey = EntityManager.GetEntity("Cubey");
		cubey->Matrix().RotateX(ToRadians(53.0f) * updateTime);
		cubey->Matrix().RotateZ(ToRadians(42.0f) * updateTime);
		cubey->Matrix().RotateWorldY(ToRadians(12.0f) * updateTime);
		Lights[1]->SetPosition(cubey->Position());

		// Move the camera
		MainCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
			CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
	}


} // namespace gen
