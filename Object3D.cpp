#include "Object3D.h"
#include "Function.h"

Object3D::Object3D(ID3D12Device* device, int lightingMode) {
    // WVPリソース作成
    wvpResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));
    TransformationMatrix* wvpData = nullptr;
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    wvpData->WVP = Math::MakeIdentity();
    wvpData->world = Math::MakeIdentity();

    // Materialリソース作成
    materialResource_ = CreateBufferResource(device, sizeof(Material));
    Material* mtl = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mtl));
    mtl->color = { 1,1,1,1 };
    mtl->enableLighting = lightingMode; // 0=なし, 1=ライト有効, 2=モデル用
    mtl->uvtransform = Math::MakeIdentity();

    // Transform 初期化
    transform_ = { {1,1,1}, {0,0,0}, {0,0,0} };
    uvTransform_ = { {1,1,1}, {0,0,0}, {0,0,0} };
}