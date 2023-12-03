#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Lights.h"
#include "Sky.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();
	void LoadTexturesAndCreateMaterials();
	void CreateLights();
	void CreateShadowMap();
	void RenderShadowMap();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr
	
	// Shaders and shader-related constructs
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> customPS;

	std::vector<std::shared_ptr<Entity>> entities;
	std::shared_ptr<Entity> floor;

	std::shared_ptr<Camera> activeCamera;
	std::vector<std::shared_ptr<Camera>> cameraList;

	std::vector<std::shared_ptr<Material>> materials;

	DirectX::XMFLOAT3 ambientColor = DirectX::XMFLOAT3(0.59f,0.42f,0.52f);

	std::vector<Light> lights;

	std::shared_ptr<Sky> sky;
	std::shared_ptr<Mesh> skyMesh;
	std::shared_ptr<SimpleVertexShader> skyVertexShader;
	std::shared_ptr<SimplePixelShader> skyPixelShader;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	std::shared_ptr<SimpleVertexShader> shadowVertexShader;
	int shadowMapResolution;
	float lightProjectionSize;
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightProjectionMatrix;
};

