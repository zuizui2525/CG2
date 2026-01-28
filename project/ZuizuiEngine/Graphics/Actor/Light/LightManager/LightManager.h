#pragma once
#include <vector>
#include <memory>
#include "Struct.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

class LightManager {
public:
    void Initialize();
    void Update();

    // ライトの追加窓口
    void AddDirectionalLight(DirectionalLightObject* light) { directionalLights_.push_back(light); }
    void AddPointLight(PointLightObject* light) { pointLights_.push_back(light); }
    void AddSpotLight(SpotLightObject* light) { spotLights_.push_back(light); }

    // GPUアドレス取得（ModelObjectのDrawで使用）
    D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightGroupAddress() const {
        return directionalLightResource_->GetGPUVirtualAddress();
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetPointLightGroupAddress() const {
        return pointLightResource_->GetGPUVirtualAddress();
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGroupAddress() const {
        return spotLightResource_->GetGPUVirtualAddress();
    }

private:
    // 平行光源(Group)用
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLightGroup* directionalLightData_ = nullptr;
    std::vector<DirectionalLightObject*> directionalLights_;

    // 点光源用
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLightGroup* pointLightData_ = nullptr;
    std::vector<PointLightObject*> pointLights_;

    // スポットライト用
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    SpotLightGroup* spotLightData_ = nullptr;
    std::vector<SpotLightObject*> spotLights_;
};
