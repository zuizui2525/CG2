#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Struct.h"
#include "Function.h"

class PointLightObject {
public:
    // 初期化（デバイス渡し）
    void Initialize(ID3D12Device* device);

    // 毎フレーム更新
    void Update();

    // ImGui操作
    void ImGuiControl();

    // GPU仮想アドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
        return resource_->GetGPUVirtualAddress();
    }

    // CPU側のライトデータへのアクセス
    PointLight* GetLightData() { return lightData_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    PointLight* lightData_ = nullptr;
};
