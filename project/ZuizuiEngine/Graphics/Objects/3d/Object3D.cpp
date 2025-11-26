#include "Object3D.h"
#include "Function.h"
#include "Matrix.h"
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

void Object3D::ImGuiSRTControl() {
    if (ImGui::CollapsingHeader("SRT")) {
        ImGui::DragFloat3("scale", &transform_.scale.x, 0.01f); // Triangleの拡縮を変更するUI
        ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f); // Triangleの回転を変更するUI
        ImGui::DragFloat3("Translate", &transform_.translate.x, 0.01f); // Triangleの位置を変更するUI
    }
    if (ImGui::CollapsingHeader("Color")) {
        ImGui::ColorEdit4("Color", &materialData_->color.x, true); // 色の値を変更するUI
    }
    ImGui::Separator();
}

void Object3D::ImGuiLightingControl() {
    if (ImGui::CollapsingHeader("lighting")) {
        ImGui::RadioButton("None", &materialData_->enableLighting, 0);
        ImGui::RadioButton("Lambert", &materialData_->enableLighting, 1);
        ImGui::RadioButton("HalfLambert", &materialData_->enableLighting, 2);
    }
    ImGui::Separator();
}
