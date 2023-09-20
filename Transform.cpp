#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
	position = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);
	rotation = XMFLOAT3(0, 0, 0);
	XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, DirectX::XMMatrixIdentity());
}

void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
}

void Transform::SetPosition(DirectX::XMFLOAT3 _position)
{
	position = _position;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	rotation.x = pitch;
	rotation.y = yaw;
	rotation.z = roll;
}

void Transform::SetRotation(DirectX::XMFLOAT3 _rotation)
{
	rotation = _rotation;
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
}

void Transform::SetScale(DirectX::XMFLOAT3 _scale)
{
	scale = _scale;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return rotation;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateMatrices();
	return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	UpdateMatrices();
	return worldInverseTranspose;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 _offset)
{
	XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&_offset)));
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	rotation.x += pitch;
	rotation.y += yaw;
	rotation.z += roll;
}

void Transform::Rotate(DirectX::XMFLOAT3 _rotation)
{
	XMStoreFloat3(&rotation, XMVectorAdd(XMLoadFloat3(&rotation), XMLoadFloat3(&_rotation)));
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
}

void Transform::Scale(DirectX::XMFLOAT3 _scale)
{
	XMStoreFloat3(&scale, XMVectorMultiply(XMLoadFloat3(&scale), XMLoadFloat3(&_scale)));
}

void Transform::UpdateMatrices()
{
	XMMATRIX t = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	XMMATRIX s = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
	XMMATRIX r = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));

	XMMATRIX worldMatrix = s * r * t;

	XMStoreFloat4x4(&world, worldMatrix);
	XMStoreFloat4x4(&worldInverseTranspose,
		XMMatrixInverse(0, XMMatrixTranspose(worldMatrix)));
}
