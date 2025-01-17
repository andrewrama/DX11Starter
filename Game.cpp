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

	shadowMapResolution = 1024;
	lightProjectionSize = 10.0f;
	lightProjectionMatrix = XMFLOAT4X4();
	lightViewMatrix = XMFLOAT4X4();
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

	SetUpRenderTarget();
	// Post process sampler state setup
	D3D11_SAMPLER_DESC ppSampDesc = {};
	ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());

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

	CreateShadowMap();
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

	shadowVertexShader = std::make_shared<SimpleVertexShader>(device, context,
		FixPath(L"ShadowVertexShader.cso").c_str());

	ppVS = std::make_shared<SimpleVertexShader>(device, context,
		FixPath(L"FullScreenVertexShader.cso").c_str());

	ppPS = std::make_shared<SimplePixelShader>(device, context,
		FixPath(L"PostProcessPixelShader.cso").c_str());
}

void Game::LoadTexturesAndCreateMaterials() 
{
#pragma region loadTextures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeAlbedoSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/bronze_albedo.png").c_str(), 0,
		bronzeAlbedoSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/bronze_normals.png").c_str(), 0,
		bronzeNormalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughnessSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/bronze_roughness.png").c_str(), 0,
		bronzeRoughnessSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeMetalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/bronze_metal.png").c_str(), 0,
		bronzeMetalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneAlbedoSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone_albedo.png").c_str(), 0,
		cobblestoneAlbedoSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneNormalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str(), 0,
		cobblestoneNormalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneRoughnessSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone_roughness.png").c_str(), 0,
		cobblestoneRoughnessSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneMetalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/cobblestone_metal.png").c_str(), 0,
		cobblestoneMetalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorAlbedoSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/floor_albedo.png").c_str(), 0,
		floorAlbedoSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorNormalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/floor_normals.png").c_str(), 0,
		floorNormalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorRoughnessSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/floor_roughness.png").c_str(), 0,
		floorRoughnessSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorMetalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/floor_metal.png").c_str(), 0,
		floorMetalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodAlbedoSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/wood_albedo.png").c_str(), 0,
		woodAlbedoSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/wood_normals.png").c_str(), 0,
		woodNormalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughnessSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/wood_roughness.png").c_str(), 0,
		woodRoughnessSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodMetalSRV;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/wood_metal.png").c_str(), 0,
		woodMetalSRV.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, sampler.GetAddressOf());

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[0]->AddSampler("BasicSampler", sampler);
	materials[0]->AddTextureSRV("Albedo", bronzeAlbedoSRV);
	materials[0]->AddTextureSRV("NormalMap", bronzeNormalSRV);
	materials[0]->AddTextureSRV("RoughnessMap", bronzeRoughnessSRV);
	materials[0]->AddTextureSRV("MetalnessMap", bronzeMetalSRV);

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[1]->AddSampler("BasicSampler", sampler);
	materials[1]->AddTextureSRV("Albedo", cobblestoneAlbedoSRV);
	materials[1]->AddTextureSRV("NormalMap", cobblestoneNormalSRV);
	materials[1]->AddTextureSRV("RoughnessMap", cobblestoneRoughnessSRV);
	materials[1]->AddTextureSRV("MetalnessMap", cobblestoneMetalSRV);

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[2]->AddSampler("BasicSampler", sampler);
	materials[2]->AddTextureSRV("Albedo", floorAlbedoSRV);
	materials[2]->AddTextureSRV("NormalMap", floorNormalSRV);
	materials[2]->AddTextureSRV("RoughnessMap", floorRoughnessSRV);
	materials[2]->AddTextureSRV("MetalnessMap", floorMetalSRV);

	materials.push_back(std::make_shared<Material>(Material(XMFLOAT3(1, 1, 1), vertexShader, pixelShader, 0.2f)));
	materials[3]->AddSampler("BasicSampler", sampler);
	materials[3]->AddTextureSRV("Albedo", woodAlbedoSRV);
	materials[3]->AddTextureSRV("NormalMap", woodNormalSRV);
	materials[3]->AddTextureSRV("RoughnessMap", woodRoughnessSRV);
	materials[3]->AddTextureSRV("MetalnessMap", woodMetalSRV);

#pragma endregion loadTextures

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

}

void Game::CreateLights()
{
	Light directionalLight1 = {};
	directionalLight1.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight1.Direction = XMFLOAT3(1.0f, -0.25f, 0.0f);
	directionalLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight1.Intensity = 1.0f;

	lights.push_back(directionalLight1);

	Light directionalLight2 = {};
	directionalLight2.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight2.Direction = XMFLOAT3(1.0f, 0.0f, 1.0f);
	directionalLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight2.Intensity = 1.0f;

	lights.push_back(directionalLight2);

	Light directionalLight3 = {};
	directionalLight3.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight3.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	directionalLight3.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directionalLight3.Intensity = 0.7f;

	lights.push_back(directionalLight3);

	Light pointLight1 = {};
	pointLight1.Type = LIGHT_TYPE_POINT;
	pointLight1.Position = XMFLOAT3(-3.0f, 1.0f, 0.0f);
	pointLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight1.Intensity = 1.0f;
	pointLight1.Range = 8.0f;

	//lights.push_back(pointLight1);

	Light pointLight2 = {};
	pointLight2.Type = LIGHT_TYPE_POINT;
	pointLight2.Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	pointLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight2.Intensity = 1.0f;
	pointLight2.Range = 8.0f;

	//lights.push_back(pointLight2);
}

void Game::CreateShadowMap()
{

	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.Height = shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());


	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTexture.Get(), &shadowDSDesc, shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture.Get(), &srvDesc, shadowSRV.GetAddressOf());

	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);


	// View
	XMMATRIX lightView = XMMatrixLookAtLH(
		XMVectorSet(0, 20, -20, 0),
		XMVectorSet(0, 0, 0, 0),
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&lightViewMatrix, lightView);

	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);
	XMStoreFloat4x4(&lightProjectionMatrix, lightProjection);
}

void Game::RenderShadowMap()
{
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);	
	ID3D11RenderTargetView* nullRTV{};
	context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());
	context->RSSetState(shadowRasterizer.Get());
	context->PSSetShader(0, 0, 0);

	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);


	shadowVertexShader->SetShader();
	shadowVertexShader->SetMatrix4x4("view", lightViewMatrix);
	shadowVertexShader->SetMatrix4x4("projection", lightProjectionMatrix);
	

	// Loop and draw all entities
	for (std::shared_ptr<Entity> e : entities)
	{
		shadowVertexShader->SetMatrix4x4("world", e->GetTransform().GetWorldMatrix());
		shadowVertexShader->CopyAllBufferData();

		e->GetMesh()->Draw(context);
	}

	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);
	context->OMSetRenderTargets(
		1,
		backBufferRTV.GetAddressOf(),
		depthBufferDSV.Get());
	context->RSSetState(0);
}

void Game::SetUpRenderTarget()
{
	ppRTV.Reset();
	ppSRV.Reset();

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = windowWidth;
	textureDesc.Height = windowHeight;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture;
	device->CreateTexture2D(&textureDesc, 0, ppTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(
		ppTexture.Get(),
		&rtvDesc,
		ppRTV.ReleaseAndGetAddressOf());

	// Create the Shader Resource View
	device->CreateShaderResourceView(
		ppTexture.Get(),
		0,
		ppSRV.ReleaseAndGetAddressOf());
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create and reposition entities
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device), materials[0])));
	entities[0]->GetTransform().SetPosition(XMFLOAT3(-3.0f, 2.0f, -2.0f));
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device), materials[1])));
	entities.push_back(std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device), materials[2])));
	entities[2]->GetTransform().SetPosition(XMFLOAT3(3.0f, 0.0f, 0.0f));

	// Floor Cube
	floor = std::make_shared<Entity>(Entity(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device), materials[3]));
	floor->GetTransform().SetScale(10.0f, 1.0f, 10.0f);
	floor->GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
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

	SetUpRenderTarget();
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

	ImGui::DragFloat("Blur", &blurRadius, 0.01f, 0.0f, 10.0f);

	ImGui::End(); // Ends the current window

	entities[0]->GetTransform().SetPosition(2.0f * sinf(totalTime * .75f) - 2.0f, 2.0f, 2.0f);
	entities[1]->GetTransform().SetPosition(0, sinf(totalTime * .75f), 0);
	entities[1]->GetTransform().Rotate(0, deltaTime * .75f, 0);
	entities[2]->GetTransform().SetPosition(3.0f, 0, 2.0f * sinf(totalTime * .75f));


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

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(ppRTV.Get(), clearColor);

	RenderShadowMap();

	context->OMSetRenderTargets(1, ppRTV.GetAddressOf(), depthBufferDSV.Get());

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	
	for(std::shared_ptr<Entity> e : entities) {
		e->GetMaterial()->GetPixelShader()->SetFloat3("cameraPos", activeCamera->GetTransform()->GetPosition());	
		vertexShader->SetMatrix4x4("lightView", lightViewMatrix);
		vertexShader->SetMatrix4x4("lightProjection", lightProjectionMatrix);

		// Add light data
		pixelShader->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		pixelShader->SetInt("lightNum", (int)lights.size());

		pixelShader->SetShaderResourceView("ShadowMap", shadowSRV);
		pixelShader->SetSamplerState("ShadowSampler", shadowSampler);

		e->Draw(context, activeCamera, totalTime);
		
	}
	floor->GetMaterial()->GetPixelShader()->SetFloat3("cameraPos", activeCamera->GetTransform()->GetPosition());
	floor->Draw(context, activeCamera, totalTime);
	sky->Draw(activeCamera);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);

	// Activate shaders and bind resources
	// Also set any required cbuffer data (not shown)	
	ppPS->SetFloat("blurRadius", blurRadius);
	ppPS->SetFloat("pixelWidth", 1.0f / windowWidth);
	ppPS->SetFloat("pixelHeight", 1.0f / windowHeight);
	ppPS->CopyAllBufferData();
	ppVS->SetShader();
	ppPS->SetShader();
	ppPS->SetShaderResourceView("Pixels", ppSRV.Get());
	ppPS->SetSamplerState("ClampSampler", ppSampler.Get());

	context->Draw(3, 0);



	ID3D11ShaderResourceView* nullSRVs[128] = {};
	context->PSSetShaderResources(0, 128, nullSRVs);

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