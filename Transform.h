#pragma once
#include "DXCore.h"
#include <DirectXMath.h>

class Transform
{
public:
	Transform();

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 _position);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 _rotation);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 _scale);

	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetScale();

	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetForward();


	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 _offset);
	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 _rotation);
	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 _scale);

private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInverseTranspose;
	DirectX::XMFLOAT3 rotation;

	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 forward;

	float matricesDirty;
	float vectorsDirty;

	void UpdateVectors();
	void UpdateMatrices();
};

