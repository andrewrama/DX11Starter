#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
	position = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);
	rotation = XMFLOAT3(0, 0, 0);
	XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, DirectX::XMMatrixIdentity());
	up = XMFLOAT3(0, 1, 0);
	right = XMFLOAT3(1, 0, 0);
	forward = XMFLOAT3(0, 0, 1);
	matricesDirty = false;
	vectorsDirty = false;
}

void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
	matricesDirty = true;
}

void Transform::SetPosition(DirectX::XMFLOAT3 _position)
{
	position = _position;
	matricesDirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	rotation.x = pitch;
	rotation.y = yaw;
	rotation.z = roll;
	matricesDirty = true;
	vectorsDirty = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 _rotation)
{
	rotation = _rotation;
	matricesDirty = true;
	vectorsDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
	matricesDirty = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 _scale)
{
	scale = _scale;
	matricesDirty = true;
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

DirectX::XMFLOAT3 Transform::GetUp()
{
	UpdateVectors();
	return up;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
	UpdateVectors();
	return right;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
	UpdateVectors();
	return forward;
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
	matricesDirty = true;
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 _offset)
{
	XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&_offset)));
	matricesDirty = true;
}

void Transform::MoveRelative(float x, float y, float z)
{
	XMVECTOR movement = XMVectorSet(x, y, z, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));

	XMVECTOR relativeDir = XMVector3Rotate(movement, rotQuat);

	XMStoreFloat3(&position, XMLoadFloat3(&position) + relativeDir);
	matricesDirty = true;
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	MoveRelative(offset.x, offset.y, offset.z);
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	rotation.x += pitch;
	rotation.y += yaw;
	rotation.z += roll;
	matricesDirty = true;
	vectorsDirty = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 _rotation)
{
	XMStoreFloat3(&rotation, XMVectorAdd(XMLoadFloat3(&rotation), XMLoadFloat3(&_rotation)));
	matricesDirty = true;
	vectorsDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
	matricesDirty = true;
}

void Transform::Scale(DirectX::XMFLOAT3 _scale)
{
	XMStoreFloat3(&scale, XMVectorMultiply(XMLoadFloat3(&scale), XMLoadFloat3(&_scale)));
	matricesDirty = true;
}

void Transform::UpdateVectors()
{
	if (!vectorsDirty) {
		return;
	}

	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
	XMStoreFloat3(&up, XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotQuat));
	XMStoreFloat3(&right, XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotQuat));
	XMStoreFloat3(&forward, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotQuat));

	vectorsDirty = false;
}

void Transform::UpdateMatrices()
{
	if (!matricesDirty) {
		return;
	}

	XMMATRIX t = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	XMMATRIX s = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
	XMMATRIX r = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));

	XMMATRIX worldMatrix = s * r * t;

	XMStoreFloat4x4(&world, worldMatrix);
	XMStoreFloat4x4(&worldInverseTranspose,
		XMMatrixInverse(0, XMMatrixTranspose(worldMatrix)));

	matricesDirty = false;
}
