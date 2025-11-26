#pragma once
#include "Function.h"
#include "Input.h"
#include "DebugCamera.h"
#include "WindowApp.h"

class Camera {
public:
    void Initialize();
    void Update(Input* input);
    void ImGuiControl();

    const Matrix4x4& GetCameraMatrix() const { return cameraMatrix_; }
    const Matrix4x4& GetViewMatrix3D() const { return viewMatrix3D_; }
    const Matrix4x4& GetProjectionMatrix3D() const { return projectionMatrix3D_; }
    const Matrix4x4& GetViewMatrix2D() const { return viewMatrix2D_; }
    const Matrix4x4& GetProjectionMatrix2D() const { return projectionMatrix2D_; }
    const Transform& GetTransform() const { return transform_; }

private:
    Transform transform_;       // 通常カメラの位置・回転・スケール
    DebugCamera debugCamera_;   // デバッグカメラ
    bool useDebugCamera_ = false;
    bool wasDebugCameraLastFrame_ = false;

    Matrix4x4 cameraMatrix_;
    Matrix4x4 viewMatrix3D_;
    Matrix4x4 projectionMatrix3D_;
    Matrix4x4 viewMatrix2D_;
    Matrix4x4 projectionMatrix2D_;
};
