#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(float x, float y, float z, 
    float _moveSpeed, 
    float _mouseLookSpeed, 
    float _fov, 
    float _aspectRatio,
    float _nearClipPlane,
    float _farClipPlane)
{
    transform.SetPosition(x, y, z);
    moveSpeed = _moveSpeed;
    mouseLookSpeed = _mouseLookSpeed;
    fov = _fov;
    aspectRatio = _aspectRatio;

    nearClipPlane = _nearClipPlane;
    farClipPlane = _farClipPlane;

    UpdateViewMatrix();
    UpdateProjectionMatrix(aspectRatio);
}

Camera::~Camera()
{
}

Transform* Camera::GetTransform()
{
    return &transform;
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
    UpdateProjectionMatrix(aspectRatio);
}

void Camera::SetNearClipPlane(float distance)
{
    nearClipPlane = distance;
    UpdateProjectionMatrix(aspectRatio);
}

void Camera::SetFarClipPlane(float distance)
{
    farClipPlane = distance;
    UpdateProjectionMatrix(aspectRatio);
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
    aspectRatio = _aspectRatio;

    XMMATRIX projection = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearClipPlane, farClipPlane);

    XMStoreFloat4x4(&projMatrix, projection);
}

void Camera::UpdateViewMatrix()
{
    XMFLOAT3 position = transform.GetPosition();
    XMFLOAT3 forward = transform.GetForward();

    // Use the position, forward, and world up to calculate view matrix
    XMMATRIX viewMat = XMMatrixLookToLH(XMLoadFloat3(&position), XMLoadFloat3(&forward), XMVectorSet(0, 1, 0, 0));

    XMStoreFloat4x4(&viewMatrix, viewMat);
}

void Camera::Update(float dt)
{
    // Get instance of input manager
    Input& input = Input::GetInstance();

    // Scale Movement speed by dt
    float ms = moveSpeed * dt;

    // Camera movement
    if (input.KeyDown('W')) 
        transform.MoveRelative(0, 0, ms);
    if (input.KeyDown('A'))
        transform.MoveRelative(-ms, 0, 0);
    if (input.KeyDown('S')) 
        transform.MoveRelative(0, 0, -ms);
    if (input.KeyDown('D'))
        transform.MoveRelative(ms, 0, 0);
    if (input.KeyDown(' '))
        transform.MoveAbsolute(0, ms, 0);
    if (input.KeyDown('X')) 
        transform.MoveAbsolute(0, -ms, 0);

    // Camera rotation
    if (input.MouseLeftDown())
    {
        float cursorMovementX = input.GetMouseXDelta() * mouseLookSpeed;
        float cursorMovementY = input.GetMouseYDelta() * mouseLookSpeed;
        transform.Rotate(cursorMovementY, cursorMovementX, 0);

        // Clamp x so that camera doesn't flip upside down
        XMFLOAT3 rotation = transform.GetPitchYawRoll();
        if (rotation.x > XM_PIDIV2)
            rotation.x = XM_PIDIV2-0.005f;
        if (rotation.x < -XM_PIDIV2)
            rotation.x = -XM_PIDIV2+0.005f;
        transform.SetRotation(rotation);
    }

    UpdateViewMatrix();
}
