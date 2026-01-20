#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Struct.h"
#include "BaseResource.h"

class Zuizui;

class Object3D : public Base3D {
public:
    Object3D() = default;
    virtual ~Object3D() = default;

    void Initialize(int lightingMode);

    // ImGui
    void ImGuiControl(const std::string& name);

    // 共通アクセサ
    // Getter
    ID3D12Resource* GetWVPResource() const { return wvpResource_.Get(); }
    ID3D12Resource* GetMaterialResource() const { return materialResource_.Get(); }
    Transform& GetTransform() { return transform_; }
    Vector3& GetScale() { return transform_.scale; }
    Vector3& GetRotate() { return transform_.rotate; }
    Vector3& GetPosition() { return transform_.translate; }
    Transform& GetUVTransform() { return uvTransform_; }
    Material* GetMaterialData() { return materialData_; }
    TransformationMatrix* GetWvpData() { return wvpData_; }

    // Setter
    void SetTransform(Transform transform) { transform_ = transform; }
    void SetScale(Vector3 scale) { transform_.scale = scale; }
    void SetRotate(Vector3 rotate) { transform_.rotate = rotate; }
    void SetPosition(Vector3 position) { transform_.translate = position; }

protected:
    // ImGui
    void ImGuiSRTControl(const std::string& name);
    void ImGuiLightingControl(const std::string& name);

    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

    // CPU 側 Transform
    Transform transform_{};
    Transform uvTransform_{};
    Material* materialData_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;
};
