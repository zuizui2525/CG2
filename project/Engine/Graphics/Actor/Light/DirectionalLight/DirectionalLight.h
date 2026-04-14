#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Struct.h"
#include "Function.h"

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
