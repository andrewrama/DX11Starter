#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "BufferStructs.h"

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

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		context->VSSetShader(vertexShader.Get(), 0, 0);
		context->PSSetShader(pixelShader.Get(), 0, 0);
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

	// Get size as the next multiple of 16
	unsigned int size = sizeof(VertexShaderExternalData);
	size = (size + 15) / 16 * 16; // This will work even if the struct size changes

	// Describe the constant buffer
	D3D11_BUFFER_DESC cbDesc = {}; // Sets struct to all zeros
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = size; // Must be a multiple of 16
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf());

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
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red	= XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green	= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);


	// Set up the vertices of the triangle we would like to draw
	// - We're going to copy this array, exactly as it exists in CPU memory
	//    over to a Direct3D-controlled data structure on the GPU (the vertex buffer)
	// - Note: Since we don't have a camera or really any concept of
	//    a "3d world" yet, we're simply describing positions within the
	//    bounds of how the rasterizer sees our screen: [-1 to +1] on X and Y
	// - This means (0,0) is at the very center of the screen.
	// - These are known as "Normalized Device Coordinates" or "Homogeneous 
	//    Screen Coords", which are ways to describe a position without
	//    knowing the exact size (in pixels) of the image/window/etc.  
	// - Long story short: Resizing the window also resizes the triangle,
	//    since we're describing the triangle in terms of the window itself
	Vertex triangleVertices[] =
	{
		{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
	};

	// Set up indices, which tell us which vertices to use and in which order
	// - This is redundant for just 3 vertices, but will be more useful later
	// - Indices are technically not required if the vertices are in the buffer 
	//    in the correct order and each one will be used exactly once
	// - But just to see how it's done...
	unsigned int triangleIndices[] = { 0, 1, 2 };

	std::shared_ptr<Mesh> triangle = std::make_shared<Mesh>(triangleVertices, 3, triangleIndices, 3, device, context);

	Vertex rectVertices[] =
	{
		{ XMFLOAT3(-0.75f, +0.3f, +0.0f), red },
		{ XMFLOAT3(-0.5f, +0.3f, +0.0f), blue },
		{ XMFLOAT3(-0.75f, -0.25f, +0.0f), green },
		{ XMFLOAT3(-0.5f, -0.25f, +0.0f), red },
	};

	unsigned int rectIndices[] = { 0, 1, 2, 2, 1, 3 };

	std::shared_ptr<Mesh> rectangle = std::make_shared<Mesh>(rectVertices, 4, rectIndices, 6, device, context);

	Vertex funVertices[] =
	{
		{ XMFLOAT3(+0.5f, -0.25f, +0.0f), red },
		{ XMFLOAT3(+0.75f, +0.3f, +0.0f), blue },
		{ XMFLOAT3(+0.6f, -0.25f, +0.0f), green },
		{ XMFLOAT3(+0.75f, +0.1f, +0.0f), blue },
		{ XMFLOAT3(+1.0f, -0.25f, +0.0f), red },
		{ XMFLOAT3(+0.9f, -0.25f, +0.0f), green },

	};

	unsigned int funIndices[] = { 0, 1, 2, 2, 1, 3, 1, 4, 5, 5, 3, 1};

	std::shared_ptr<Mesh> fun = std::make_shared<Mesh>(funVertices, 6, funIndices, 12, device, context);

	entities.push_back(std::make_shared<Entity>(Entity(triangle)));
	entities.push_back(std::make_shared<Entity>(Entity(rectangle)));
	entities.push_back(std::make_shared<Entity>(Entity(fun)));
	entities.push_back(std::make_shared<Entity>(Entity(fun)));
	entities.push_back(std::make_shared<Entity>(Entity(rectangle)));
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
	ImGui::ColorEdit4("Tint", &meshTint.x);

	// Entity UI
	if (ImGui::TreeNode("Entities"))
	{
		for (int i = 0; i < entities.size(); i++)
		{
			entities[i]->SetTint(meshTint);
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

	ImGui::End(); // Ends the current window

	//Apply Transformations to Entities
	entities[3]->GetTransform().Rotate(0, 0, sinf(deltaTime));
	entities[3]->GetTransform().Scale(0.99999f, 0.99999f, 1.0f);
	entities[0]->GetTransform().MoveAbsolute(XMFLOAT3(0,sinf(deltaTime), 0));
	entities[4]->GetTransform().SetPosition(XMFLOAT3(0, sinf(totalTime), 0));
	entities[1]->GetTransform().SetScale(sinf(totalTime), sinf(totalTime), 1);
	entities[2]->GetTransform().SetPosition((XMFLOAT3(sinf(totalTime)-0.5f, sinf(totalTime), 0)));

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
		e->Draw(context, vsConstantBuffer, activeCamera);
	}
	

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