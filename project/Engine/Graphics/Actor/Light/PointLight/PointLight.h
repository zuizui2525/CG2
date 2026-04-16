#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Math/MathStructs.h"

static const int kMaxPointLights = 10;

struct PointLight {
    Vector4 color;     //!< ライトの色
    Vector3 position;  //!< ライトの座標
    float intensity;   //!< 輝度
    float radius;      //!< ライトの届く最大距離
    float decay;       //!< 減衰率（値が大きいほど急激に暗くなる）
    float padding[2];  //!< 16バイトアライメントのためのパディング
};

struct PointLightGroup {
    PointLight lights[kMaxPointLights]; // 10個の配列
    int32_t numLights;                  // 実際に使う数
    float padding[3];                   // 16バイトアライメントのための隙間
};

class PointLightObject {
public:
    // 初期化
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
