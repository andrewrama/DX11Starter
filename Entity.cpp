#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> _mesh , std::shared_ptr<Material> _material):
	mesh(_mesh),
	material(_material)
{
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

std::shared_ptr<Material> Entity::GetMaterial()
{
	return material;
}


void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	std::shared_ptr<Camera> camera, float totalTime)
{
	material->PrepareMaterial();

	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	vs->SetMatrix4x4("world", transform.GetWorldMatrix());
	vs->SetMatrix4x4("worldInverseTranspose", transform.GetWorldInverseTransposeMatrix());
	vs->SetMatrix4x4("view", camera->GetViewMatrix());
	vs->SetMatrix4x4("projection", camera->GetProjectionMatrix());
	vs->CopyAllBufferData();

	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();
	ps->SetFloat("totalTime", totalTime);
	ps->CopyAllBufferData();

	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();

	mesh->Draw(context);
}

void Entity::SetMesh(std::shared_ptr<Mesh> _mesh)
{
	mesh = _mesh;
}

void Entity::SetMaterial(std::shared_ptr<Material> _material)
{
	material = _material;
}
