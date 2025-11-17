#include "Object3D.h"
#include "../Function/Function.h"
#include "../Matrix/Matrix.h"
#include <stdexcept>

Object3D::Object3D(ID3D12Device* device, int lightingMode) {
    // WVPリソース作成
    wvpResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));
    if (!wvpResource_) throw std::runtime_error("Failed to create wvpResource_");
    HRESULT hr = wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    if (FAILED(hr) || !wvpData_) throw std::runtime_error("Failed to map wvpResource_");
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Materialリソース作成
    materialResource_ = CreateBufferResource(device, sizeof(Material));
    if (!materialResource_) throw std::runtime_error("Failed to create materialResource_");
    hr = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    if (FAILED(hr) || !materialData_) throw std::runtime_error("Failed to map materialResource_");
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = lightingMode;
    materialData_->uvtransform = Math::MakeIdentity();

    // Transform初期化
    transform_.scale = { 1,1,1 };
    transform_.rotate = { 0,0,0 };
    uvTransform_ = { {1,1,1}, {0,0,0}, {0,0,0} };
}
