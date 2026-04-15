#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Struct.h"

static const int kMaxDirectionalLights = 2;

struct DirectionalLight {
    Vector4 color;     //!< ライトの色
    Vector3 direction; //!< ライトの方向
    float intensity;   //!< ライトの強度
};

struct DirectionalLightGroup {
    DirectionalLight lights[kMaxDirectionalLights]; // 配列
    int32_t numLights;                              // 有効なライト数
    float padding[3];                               // アライメント調整
};

class DirectionalLightObject {
public:
    // 初期化
    void Initialize();

    // 毎フレーム更新（正規化とか）
    void Update();

    // ImGui操作
    void ImGuiControl(const std::string& name);

    // 実体の参照を返す
    DirectionalLight& GetLightData() { return data_; }

private:
    DirectionalLight data_;

    bool isWindowOpen_ = false;
};
