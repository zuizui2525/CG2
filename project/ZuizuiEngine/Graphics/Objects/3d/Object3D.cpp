#include "Object3D.h"
#include "Function.h"
#include "Matrix.h"
#include "Zuizui.h"
#include <stdexcept>

void Object3D::Initialize(Zuizui* engine, int lightingMode) {
    // ポインタに代入
    engine_ = engine;
    // WVPリソース作成
    wvpResource_ = CreateBufferResource(engine_->GetDevice(), sizeof(TransformationMatrix));
    if (!wvpResource_) throw std::runtime_error("Failed to create wvpResource_");
    HRESULT hr = wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    if (FAILED(hr) || !wvpData_) throw std::runtime_error("Failed to map wvpResource_");
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Materialリソース作成
    materialResource_ = CreateBufferResource(engine_->GetDevice(), sizeof(Material));
    if (!materialResource_) throw std::runtime_error("Failed to create materialResource_");
    hr = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    if (FAILED(hr) || !materialData_) throw std::runtime_error("Failed to map materialResource_");
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = lightingMode;
    materialData_->uvtransform = Math::MakeIdentity();
    materialData_->shininess = 30.0f;

    // Transform初期化
    transform_.scale = { 1,1,1 };
    transform_.rotate = { 0,0,0 };
    uvTransform_ = { {1,1,1}, {0,0,0}, {0,0,0} };
}

void Object3D::ImGuiControl(const std::string& name) {
    ImGuiSRTControl(name);
    ImGuiLightingControl(name);
    ImGui::Separator();
}

void Object3D::ImGuiSRTControl(const std::string& name) {
    std::string label = "##" + name;
    
    if (ImGui::CollapsingHeader(("SRT" + label).c_str())) {
        ImGui::DragFloat3(("scale" + label).c_str(), &transform_.scale.x, 0.01f);
        ImGui::DragFloat3(("rotate" + label).c_str(), &transform_.rotate.x, 0.01f);
        ImGui::DragFloat3(("Translate" + label).c_str(), &transform_.translate.x, 0.01f);
    }
    if (ImGui::CollapsingHeader(("Color" + label).c_str())) {
       ImGui::ColorEdit4(("Color" + label).c_str(), &materialData_->color.x, true);
    }
}

void Object3D::ImGuiLightingControl(const std::string& name) {
    std::string label = "##" + name;

    if (ImGui::CollapsingHeader(("lighting" + label).c_str())) {
        ImGui::RadioButton(("None" + label).c_str(), &materialData_->enableLighting, 0);
        ImGui::RadioButton(("Lambert" + label).c_str(), &materialData_->enableLighting, 1);
        ImGui::RadioButton(("HalfLambert" + label).c_str(), &materialData_->enableLighting, 2);
    }
}
