#include "Camera.h"

using namespace DirectX;

Camera::Camera(float x, float y, float z, 
    float _moveSpeed, 
    float _mouseLookSpeed, 
    float _fov, 
    float _aspectRatio)
{
    transform.SetPosition(x, y, z);
    moveSpeed = _moveSpeed;
    mouseLookSpeed = _mouseLookSpeed;
    fov = _fov;
    aspectRatio = _aspectRatio;

    UpdateViewMatrix();
    UpdateProjectionMatrix(aspectRatio);
}

Camera::~Camera()
{
}

Transform& Camera::GetTransform()
{
    return transform;
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
    return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
    return projMatrix;
}

float Camera::GetAspectRatio()
{
    return aspectRatio;
}

float Camera::GetFieldOfView()
{
    return fov;
}

float Camera::GetNearClipPlane()
{
    return nearClipPlane;
}

float Camera::GetFarClipPlane()
{
    return farClipPlane;
}

float Camera::GetMoveSpeed()
{
    return moveSpeed;
}

float Camera::GetMouseLookSpeed()
{
    return mouseLookSpeed;
}

void Camera::SetFieldOfView(float _fov)
{
    fov = _fov;
}

void Camera::SetNearClipPlane(float distance)
{
    nearClipPlane = distance;
}

void Camera::SetFarClipPlane(float distance)
{
    farClipPlane = distance;
}

void Camera::SetMoveSpeed(float speed)
{
    moveSpeed = speed;
}

void Camera::SetMouseLookSpeed(float speed)
{
    mouseLookSpeed = speed;
}

void Camera::UpdateProjectionMatrix(float _aspectRatio)
{
}

void Camera::UpdateViewMatrix()
{
}

void Camera::Update(float dt)
{
}
