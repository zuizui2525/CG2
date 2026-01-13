#pragma once
#include "Function.h"
#include "Input.h"
#include "DebugCamera.h"
#include "WindowApp.h"
#include <d3d12.h>
#include <wrl.h>

class Camera {
public:
    void Initialize(ID3D12Device* device, Input* input);
    void Update();
    void ImGuiControl();

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return resource_->GetGPUVirtualAddress(); }
    const Matrix4x4& GetCameraMatrix() const { return cameraMatrix_; }
    const Matrix4x4& GetViewMatrix3D() const { return viewMatrix3D_; }
    const Matrix4x4& GetProjectionMatrix3D() const { return projectionMatrix3D_; }
    const Matrix4x4& GetViewMatrix2D() const { return viewMatrix2D_; }
    const Matrix4x4& GetProjectionMatrix2D() const { return projectionMatrix2D_; }
    const Transform& GetTransform() const { return transform_; }

    void SetTransform(const Transform& transform) { transform_ = transform; }
    void SetRotation(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetPosition(const Vector3& position) { transform_.translate = position; }

private:
    Transform transform_;       // 通常カメラの位置・回転・スケール
    DebugCamera debugCamera_;   // デバッグカメラ
    bool useDebugCamera_ = false;
    bool wasDebugCameraLastFrame_ = false;

    Input* input_ = nullptr;

    Matrix4x4 cameraMatrix_;
    Matrix4x4 viewMatrix3D_;
    Matrix4x4 projectionMatrix3D_;
    Matrix4x4 viewMatrix2D_;
    Matrix4x4 projectionMatrix2D_;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    CameraForGPU* data_ = nullptr;
};
