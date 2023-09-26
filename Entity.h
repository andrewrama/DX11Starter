#pragma once
#include <memory>
#include "Transform.h"
#include "Mesh.h"
class Entity
{
public:
	Entity(std::shared_ptr<Mesh> _mesh);

	std::shared_ptr<Mesh> GetMesh();
	Transform& GetTransform();
	void SetTint(DirectX::XMFLOAT4 tint);
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
		Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer,
		std::shared_ptr<Camera> camera);
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	DirectX::XMFLOAT4 meshTint = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
};

