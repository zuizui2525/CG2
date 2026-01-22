#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Struct.h"
#include "Function.h"

class SpotLightObject {
public:
    // リソースの生成と初期値の設定
    void Initialize(ID3D12Device* device);

    // データの更新（方向の正規化や角度の計算）
    void Update();

    // デバッグ用のUI操作
    void ImGuiControl(const std::string& name);

    // 描画時に使用するGPUアドレスの取得
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
        return resource_->GetGPUVirtualAddress();
    }

    // 生データへのポインタ取得
    SpotLight* GetLightData() { return lightData_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    SpotLight* lightData_ = nullptr;

    // 内部調整用変数（度数法で保持）
    float inputAngle_ = 45.0f;
    float inputFalloffStart_ = 30.0f;

    // ImGuiウィンドウの開閉状態
    bool isWindowOpen_ = false;
};
