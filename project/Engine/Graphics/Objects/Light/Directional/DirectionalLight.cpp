#include "Engine/Graphics/Objects/Light/Directional/DirectionalLight.h"
#include "ImGuiManager.h"
#include "Engine/Math/Matrix/Matrix.h"

void DirectionalLightObject::Initialize() {
    data_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    data_.direction = { 0.0f, -1.0f, 0.0f };
    data_.intensity = 1.0f;

    // ヒエラルキー自動登録
    InitializeGameObject("DirectionalLight");
}

DirectionalLightObject::~DirectionalLightObject() = default;

void DirectionalLightObject::Update() {
    // 毎フレーム方向を正規化して安全に保つ
    data_.direction = Math::Normalize(data_.direction);
}
void DirectionalLightObject::DrawInspector() {
#ifdef _USEIMGUI
    std::string label = "##" + name_;
    if (ImGui::CollapsingHeader(("Directional Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit4(("Color" + label).c_str(), &data_.color.x);
        ImGui::DragFloat3(("Direction" + label).c_str(), &data_.direction.x, 0.01f, 0.0f, 0.0f, "%.1f");
        ImGui::DragFloat(("Intensity" + label).c_str(), &data_.intensity, 0.01f, 0.0f, 0.0f, "%.1f");
    }
#endif
}
