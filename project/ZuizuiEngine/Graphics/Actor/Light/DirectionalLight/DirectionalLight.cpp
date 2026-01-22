#include "DirectionalLight.h"
#include "Matrix.h"

void DirectionalLightObject::Initialize(ID3D12Device* device) {
    // リソースを作成
    resource_ = CreateBufferResource(device, sizeof(DirectionalLight));

    // マップしてポインタを取得
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

    // 初期値
    lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->direction = { 0.0f, -1.0f, 0.0f };
    lightData_->intensity = 1.0f;

    // Unmap はしない（ずっと書き換えるため）
}

void DirectionalLightObject::Update() {
    // 毎フレーム方向を正規化して安全に保つ
    lightData_->direction = Math::Normalize(lightData_->direction);
}

void DirectionalLightObject::ImGuiControl(const std::string& name) {
    ImGui::Begin("Light List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            std::string label = "##" + name;

            if (ImGui::CollapsingHeader(("Directional Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4(("Color" + label).c_str(), &lightData_->color.x);
                ImGui::DragFloat3(("Direction" + label).c_str(), &lightData_->direction.x, 0.01f);
                ImGui::DragFloat(("Intensity" + label).c_str(), &lightData_->intensity, 0.01f);
            }
        }
        ImGui::End();
    }
}
