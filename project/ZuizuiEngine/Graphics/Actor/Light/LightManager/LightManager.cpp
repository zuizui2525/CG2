#include "LightManager.h"
#include "Function.h"
#include "BaseResource.h"
#include "Zuizui.h"

void LightManager::Initialize() {
    auto device = EngineResource::GetEngine()->GetDevice();

    // 点光源グループのバッファ作成
    pointLightResource_ = CreateBufferResource(device, sizeof(PointLightGroup));
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

    // スポットライトグループのバッファ作成
    spotLightResource_ = CreateBufferResource(device, sizeof(SpotLightGroup));
    spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));
}

void LightManager::Update() {
    // 点光源のデータを集計
    pointLightData_->numLights = static_cast<int32_t>((std::min)((int)pointLights_.size(), kMaxPointLights));
    for (int i = 0; i < pointLightData_->numLights; ++i) {
        pointLightData_->lights[i] = pointLights_[i]->GetLightData();
    }

    // スポットライトのデータを集計
    spotLightData_->numLights = static_cast<int32_t>((std::min)((int)spotLights_.size(), kMaxSpotLights));
    for (int j = 0; j < spotLightData_->numLights; ++j) {
        spotLightData_->lights[j] = spotLights_[j]->GetLightData();
    }
}
