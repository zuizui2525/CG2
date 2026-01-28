#include "DirectionalLight.h"
#include "Matrix.h"

void DirectionalLightObject::Initialize() {
    data_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    data_.direction = { 0.0f, -1.0f, 0.0f };
    data_.intensity = 1.0f;
}

void DirectionalLightObject::Update() {
    // 毎フレーム方向を正規化して安全に保つ
    data_.direction = Math::Normalize(data_.direction);
}

void DirectionalLightObject::ImGuiControl(const std::string& name) {
    ImGui::Begin("Light List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            std::string label = "##" + name;

            if (ImGui::CollapsingHeader(("Directional Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4(("Color" + label).c_str(), &data_.color.x);
                ImGui::DragFloat3(("Direction" + label).c_str(), &data_.direction.x, 0.01f);
                ImGui::DragFloat(("Intensity" + label).c_str(), &data_.intensity, 0.01f);
            }
        }
        ImGui::End();
    }
}
