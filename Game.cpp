#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Material.h"


#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "WICTextureLoader.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs


	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	LoadTexturesAndCreateMaterials();
	CreateLights();
	CreateGeometry();
	
	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Create 3 different cameras
	cameraList.push_back(std::make_shared<Camera>(Camera(0.0f, 0.0f, -5.0f, 4.0f, 0.006f, XM_PIDIV4, (float)this->windowWidth / this->windowHeight, 0.0001f, 100.0f)));
	cameraList.push_back(std::make_shared<Camera>(Camera(0.0f, 5.0f, -10.0f, 4.0f, 0.006f, XM_PIDIV2, (float)this->windowWidth / this->windowHeight, 0.0001f, 100.0f)));
	cameraList.push_back(std::make_shared<Camera>(Camera(2.0f, 2.0f, -6.0f, 4.0f, 0.006f, XM_PI/3, (float)this->windowWidth / this->windowHeight, 0.0001f, 100.0f)));

	// Set active camera as first camera in the list
	activeCamera = cameraList[0];
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(device, context,
		FixPath(L"VertexShader.cso").c_str());

	pixelShader = std::make_shared<SimplePixelShader>(device, context,
		FixPath(L"PixelShader.cso").c_str());

	customPS = std::make_shared<SimplePixelShader>(device, context,
		FixPath(L"CustomPS.cso").c_str());

	skyVertexShader = std::make_shared<SimpleVertexShader>(device, context,
		FixPath(L"SkyVertexShader.cso").c_str());

	skyPixelShader = std::make_shared<SimplePixelShader>(device, context,
		FixPath(L"SkyPixelShader.cso").c_str());
}

void Game::LoadTexturesAndCreateMaterials() 
{
#pragma region loadTextures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone.png").c_str(), 0,
		cobblestoneSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneNormalsSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str(), 0,
		cobblestoneNormalsSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneSpecSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone_specular.png").c_str(), 0,
		cobblestoneSpecSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cushion.png").c_str(), 0,
		cushionSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionNormalsSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cushion_normals.png").c_str(), 0,
		cushionNormalsSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionSpecSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cushion_specular.png").c_str(), 0,
		cushionSpecSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/rock.png").c_str(), 0,
		rockSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockNormalsSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/rock_normals.png").c_str(), 0,
		rockNormalsSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockSpecSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/rock_specular.png").c_str(), 0,
		rockSpecSRV.GetAddressOf());

#pragma endregion loadTextures

	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, sampler.GetAddressOf());

	// Create Sky
	skyMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);

	sky = std::make_shared<Sky>(
		FixPath(L"../../Assets/Textures/Clouds Pink/right.png").c_str(),
		FixPath(L"../../Assets/Textures/Clouds Pink/left.png").c_str(),
		FixPath(L"../../Assets/Textures/Clouds Pink/up.png").c_str(),
		FixPath(L"../../Assets/Textures/Clouds Pink/down.png").c_str(),
		FixPath(L"../../Assets/Textures/Clouds Pink/front.png").c_str(),
		FixPath(L"../../Assets/Textures/Clouds Pink/back.png").c_str(),
		skyMesh,
		sampler,
		skyPixelShader,
		skyVertexShader,
		context,
		device
	);

	/*
	OLD MATERIALS

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1.0f, 1.0f, 1.0f), vertexShader, pixelShader, 0.2f)));
	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(0.6f, 0.2f, 0.2f), vertexShader, pixelShader, 0.5f)));
	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(0.5f, 0.0f, 1.0f), vertexShader, pixelShader, 0.0f)));
	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1.0f, 1.0f, 1.0f), vertexShader, customPS, 0.0f)));
	*/



	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[0]->AddSampler("BasicSampler", sampler);
	materials[0]->AddTextureSRV("SurfaceTexture", cushionSRV);
	materials[0]->AddTextureSRV("SpecularMap", cushionSpecSRV);
	materials[0]->AddTextureSRV("NormalMap", cushionNormalsSRV);

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[1]->AddSampler("BasicSampler", sampler);
	materials[1]->AddTextureSRV("SurfaceTexture", cobblestoneSRV);
	materials[1]->AddTextureSRV("SpecularMap", cobblestoneSpecSRV);
	materials[1]->AddTextureSRV("NormalMap", cobblestoneNormalsSRV);

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[2]->AddSampler("BasicSampler", sampler);
	materials[2]->AddTextureSRV("SurfaceTexture", rockSRV);
	materials[2]->AddTextureSRV("SpecularMap", rockSpecSRV);
	materials[2]->AddTextureSRV("NormalMap", rockNormalsSRV);
}

void Game::CreateLights()
{
	Light directionalLight1 = {};
	directionalLight1.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight1.Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);
	directionalLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight1.Intensity = 1.0f;

	lights.push_back(directionalLight1);

	Light directionalLight2 = {};
	directionalLight2.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight2.Direction = XMFLOAT3(-1.0f, 0.0f, 1.0f);
	directionalLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight2.Intensity = 1.0f;

	lights.push_back(directionalLight2);

	Light directionalLight3 = {};
	directionalLight3.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight3.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	directionalLight3.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight3.Intensity = 0.7f;

	//lights.push_back(directionalLight3);

	Light pointLight1 = {};
	pointLight1.Type = LIGHT_TYPE_POINT;
	pointLight1.Position = XMFLOAT3(0.0f, 0.0f, 3.0f);
	pointLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight1.Intensity = 1.0f;
	pointLight1.Range = 8.0f;

	//lights.push_back(pointLight1);

	Light pointLight2 = {};
	pointLight2.Type = LIGHT_TYPE_POINT;
	pointLight2.Position = XMFLOAT3(0.0f, 4.0f, -3.0f);
	pointLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight2.Intensity = 1.0f;
	pointLight2.Range = 8.0f;

	lights.push_back(pointLight2);
}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create and reposition entities
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device), materials[2])));
	entities[0]->GetTransform().SetPosition(XMFLOAT3(-3.0f, 0.0f, 0.0f));
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device), materials[1])));
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str(), device), materials[1])));
	entities[2]->GetTransform().SetPosition(XMFLOAT3(3.0f, 0.0f, 0.0f));
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device), materials[0])));
	entities[3]->GetTransform().SetPosition(XMFLOAT3(-6.0f, 0.0f, 0.0f));
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device), materials[2])));
	entities[4]->GetTransform().SetPosition(XMFLOAT3(6.0f, 0.0f, 0.0f));
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	// Update all camera projection matrices
	for (auto& camera : cameraList) {
		camera->UpdateProjectionMatrix((float)this->windowWidth / this->windowHeight);
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Window Info"); // Everything after is part of the window
	ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
	ImGui::Text("Width: %i", windowWidth);
	ImGui::Text("Height: %i", windowHeight);
	//ImGui::DragFloat3("Offset", &meshOffset._41, 0.01f);
	//ImGui::ColorEdit4("Tint", &meshTint.x);

	// Entity UI
	if (ImGui::TreeNode("Entities"))
	{
		for (int i = 0; i < entities.size(); i++)
		{
			ImGui::PushID(i);

			ImGui::Text("Entity %i", i);

			XMFLOAT3 pos = entities[i]->GetTransform().GetPosition();
			if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
			{
				entities[i]->GetTransform().SetPosition(pos);
			}

			XMFLOAT3 rot = entities[i]->GetTransform().GetPitchYawRoll();
			if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f))
			{
				entities[i]->GetTransform().SetRotation(rot);
			}

			XMFLOAT3 scale = entities[i]->GetTransform().GetScale();
			if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
			{
				entities[i]->GetTransform().SetScale(scale);
			}

			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	// Camera UI
	if (ImGui::TreeNode("Cameras"))
	{
		if (ImGui::RadioButton("Camera #1", activeCamera == cameraList[0])) activeCamera = cameraList[0];
		if (ImGui::RadioButton("Camera #2", activeCamera == cameraList[1])) activeCamera = cameraList[1];
		if (ImGui::RadioButton("Camera #3", activeCamera == cameraList[2])) activeCamera = cameraList[2];

		ImGui::Text("Position: %f, %f, %f", activeCamera->GetTransform()->GetPosition().x, 
			activeCamera->GetTransform()->GetPosition().y,
			activeCamera->GetTransform()->GetPosition().z);
		ImGui::Text("Field of View: %f", XMConvertToDegrees(activeCamera->GetFieldOfView()));
		ImGui::Text("Near/Far Clip Plane: %f/%f", activeCamera->GetNearClipPlane(), activeCamera->GetFarClipPlane());
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Lights")) 
	{
		for (int i = 0; i < lights.size(); i++) {
			ImGui::PushID(i);

			ImGui::Text("Light %i", i);

			ImGui::DragFloat3("Color", &lights[i].Color.x, 0.01f, 0.0f, 1.0f);

			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::End(); // Ends the current window

	entities[0]->GetTransform().Rotate(0, deltaTime * .5f, 0);
	entities[1]->GetTransform().Rotate(0, deltaTime * .5f, 0);
	entities[2]->GetTransform().Rotate(0, deltaTime * .5f, 0);
	entities[3]->GetTransform().Rotate(0, deltaTime * .5f, 0);
	entities[4]->GetTransform().Rotate(0, deltaTime * .5f, 0);

	// Determine new input capture
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);

	// Update the active camera
	activeCamera->Update(deltaTime);

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	
	for(std::shared_ptr<Entity> e : entities) {
		e->GetMaterial()->GetPixelShader()->SetFloat("roughness", e->GetMaterial()->GetRoughness());
		e->GetMaterial()->GetPixelShader()->SetFloat3("cameraPos", activeCamera->GetTransform()->GetPosition());
		e->GetMaterial()->GetPixelShader()->SetFloat3("ambientColor", ambientColor);
		
		// Add light data
		pixelShader->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		e->Draw(context, activeCamera, totalTime);
	}
	
	sky->Draw(activeCamera);

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}