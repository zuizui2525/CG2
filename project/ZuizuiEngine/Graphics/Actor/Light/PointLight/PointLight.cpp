#include "PointLight.h"
#include "imgui.h" // ImGuiを使う場合

void PointLightObject::Initialize() {
    data_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    data_.position = { 0.0f, 2.0f, 0.0f };
    data_.intensity = 1.0f;
    data_.radius = 10.0f;
    data_.decay = 1.0f;
}

void PointLightObject::Update() {
    // 点光源は方向の正規化などが不要なので、今は空でもOK
}

void PointLightObject::ImGuiControl(const std::string& name) {
    ImGui::Begin("Light List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            std::string label = "##" + name;

            if (ImGui::CollapsingHeader(("PointLight Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4(("Color" + label).c_str(), &data_.color.x);
                ImGui::DragFloat3(("Position" + label).c_str(), &data_.position.x, 0.1f);
                ImGui::DragFloat(("Intensity" + label).c_str(), &data_.intensity, 0.01f);
                ImGui::DragFloat(("Radius" + label).c_str(), &data_.radius, 0.1f);
                ImGui::DragFloat(("Decay" + label).c_str(), &data_.decay, 0.01f);
            }
        }
        ImGui::End();
    }
}
