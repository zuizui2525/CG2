#pragma once
#include <vector>
#include <memory>
#include "Struct.h"
#include "PointLight.h"
#include "SpotLight.h"

class LightManager {
public:
    void Initialize();
    void Update();

    // ライトの追加窓口
    void AddPointLight(PointLightObject* light) { pointLights_.push_back(light); }
    void AddSpotLight(SpotLightObject* light) { spotLights_.push_back(light); }

    // GPUアドレス取得（ModelObjectのDrawで使用）
    D3D12_GPU_VIRTUAL_ADDRESS GetPointLightGroupAddress() const {
        return pointLightResource_->GetGPUVirtualAddress();
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGroupAddress() const {
        return spotLightResource_->GetGPUVirtualAddress();
    }

private:
    // 点光源用
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLightGroup* pointLightData_ = nullptr;
    std::vector<PointLightObject*> pointLights_;

    // スポットライト用
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    SpotLightGroup* spotLightData_ = nullptr;
    std::vector<SpotLightObject*> spotLights_;
};
