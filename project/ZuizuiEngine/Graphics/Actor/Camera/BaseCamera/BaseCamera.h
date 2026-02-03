#pragma once
#include "Struct.h"
#include "Matrix.h"

class BaseCamera {
public:
    virtual ~BaseCamera() = default;

    virtual void Initialize();
    virtual void Update();

    // 座標・回転の操作
    void SetPosition(const Vector3& pos) { transform_.translate = pos; }
    const Vector3& GetPosition() const { return transform_.translate; }
    void SetRotation(const Vector3& rot) { transform_.rotate = rot; }
    const Vector3& GetRotation() const { return transform_.rotate; }

    // 注視点(Target)の操作
    void SetTarget(const Vector3& target) {
        target_ = target;
        useTarget_ = true;
    }
    void DisableTarget() { useTarget_ = false; }

    // 行列取得
    const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

protected:
    Transform transform_;
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;

    Vector3 target_ = { 0.0f, 0.0f, 0.0f };
    bool useTarget_ = false;
};
