#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Struct.h"
#include "Function.h"

class SpotLightObject {
public:
    // リソースの生成と初期値の設定
    void Initialize();

    // データの更新（方向の正規化や角度の計算）
    void Update();

    // デバッグ用のUI操作
    void ImGuiControl(const std::string& name);

    // 実体の参照を返す
    SpotLight& GetLightData() { return data_; }

    // Getter
    const Vector3& GetPosition() const { return data_.position; }
    const Vector3& GetDirection() const { return data_.direction; }
    float GetIntensity() const { return data_.intensity; }
    float GetAngle() const { return inputAngle_; }

    // Setter
    void SetPosition(const Vector3& position) { data_.position = position; }
    void SetDirection(const Vector3& direction) { data_.direction = direction; }
    void SetIntensity(float intensity) { data_.intensity = intensity; }
    void SetDistance(float distance) { data_.distance = distance; }

private:
    SpotLight data_;

    float inputAngle_ = 45.0f;
    float inputFalloffStart_ = 30.0f;
    bool isWindowOpen_ = false;
};
