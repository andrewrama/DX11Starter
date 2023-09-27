#pragma once
#include "DXCore.h"
#include <DirectXMath.h>
#include "Transform.h"

class Camera
{
public:
	Camera(
		float x, float y, float z,
		float _moveSpeed,
		float _mouseLookSpeed,
		float _fov,
		float _aspectRatio,
		float _nearClipPlane,
		float _farClipPlane);
	~Camera();

	Transform* GetTransform();
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	float GetAspectRatio();
	float GetFieldOfView();
	float GetNearClipPlane();
	float GetFarClipPlane();
	float GetMoveSpeed();
	float GetMouseLookSpeed();


	void SetFieldOfView(float _fov);
	void SetNearClipPlane(float distance);
	void SetFarClipPlane(float distance);
	void SetMoveSpeed(float speed);
	void SetMouseLookSpeed(float speed);


	void UpdateProjectionMatrix(float _aspectRatio);
	void UpdateViewMatrix();
	void Update(float dt);

private:
	Transform transform;

	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;

	float fov;
	float aspectRatio;

	float nearClipPlane;
	float farClipPlane;

	float moveSpeed;
	float mouseLookSpeed;
};

