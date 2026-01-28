#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Struct.h"
#include "Function.h"

class PointLightObject {
public:
    // 初期化（デバイス渡し）
    void Initialize();

    // 毎フレーム更新
    void Update();

    // ImGui操作
    void ImGuiControl(const std::string& name);

    // 実体の参照を返す
    PointLight& GetLightData() { return data_; }

    // Getter
    const Vector3& GetPosition() const { return data_.position; }
    const Vector4& GetColor() const { return data_.color; }
    float GetIntensity() const { return data_.intensity; }
    float GetRadius() const { return data_.radius; }

    // Setter
    void SetPosition(const Vector3& position) { data_.position = position; }
    void SetColor(const Vector4& color) { data_.color = color; }
    void SetIntensity(float intensity) { data_.intensity = intensity; }
    void SetRadius(float radius) { data_.radius = radius; }
    void SetDecay(float decay) { data_.decay = decay; }

private:
    PointLight data_;

    bool isWindowOpen_ = false;
};
