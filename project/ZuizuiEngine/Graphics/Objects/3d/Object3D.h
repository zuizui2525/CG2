#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Struct.h"

// 3Dオブジェクトの基底クラス
class Object3D {
public:
    Object3D(ID3D12Device* device, int lightingMode);
    virtual ~Object3D() = default;

    // ImGui
    void ImGuiControl();

    // 共通アクセサ
    ID3D12Resource* GetWVPResource() const { return wvpResource_.Get(); }
    ID3D12Resource* GetMaterialResource() const { return materialResource_.Get(); }
    Transform& GetTransform() { return transform_; }
    Transform& GetUVTransform() { return uvTransform_; }
    Material* GetMaterialData() { return materialData_; }
    TransformationMatrix* GetWvpData() { return wvpData_; }

protected:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

    // CPU 側 Transform
    Transform transform_{};
    Transform uvTransform_{};
    Material* materialData_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;
};
