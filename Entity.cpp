#include "Entity.h"
#include "BufferStructs.h"
#include "Camera.h"

Entity::Entity(std::shared_ptr<Mesh> _mesh)
{
    mesh = _mesh;
	transform = Transform();
}

std::shared_ptr<Mesh> Entity::GetMesh()
{
    return mesh;
}

Transform& Entity::GetTransform()
{
    return transform;
}

void Entity::SetTint(DirectX::XMFLOAT4 tint)
{
	meshTint = tint;
}


void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer,
	std::shared_ptr<Camera> camera)
{
	VertexShaderExternalData vsData;
	vsData.colorTint = meshTint;
	vsData.world = transform.GetWorldMatrix();
	vsData.viewMatrix = camera->GetViewMatrix();
	vsData.projMatrix = camera->GetProjectionMatrix();

	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
	context->Unmap(constantBuffer.Get(), 0);

	context->VSSetConstantBuffers(
		0,
		1,
		constantBuffer.GetAddressOf());

	mesh->Draw();
}
