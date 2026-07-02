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
    // rotation_（オイラー角）に基づいてライトの方向ベクトルを算出する
    // 初期方向を真下 {0, -1, 0} とし、回転行列を掛ける
    Matrix4x4 rotMat = Math::MakeRotateMatrix(rotation_.x, rotation_.y, rotation_.z);
    Vector3 dir = Math::TransformNormal({ 0.0f, -1.0f, 0.0f }, rotMat);
    data_.direction = Math::Normalize(dir);
}
void DirectionalLightObject::DrawInspector() {
#ifdef _USEIMGUI
    std::string label = "##" + name_;
    if (ImGui::CollapsingHeader(("Directional Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3(("Position" + label).c_str(), &position_.x, 0.1f);
        ImGui::DragFloat3(("Rotation" + label).c_str(), &rotation_.x, 0.01f);
        ImGui::ColorEdit4(("Color" + label).c_str(), &data_.color.x);
        ImGui::DragFloat(("Intensity" + label).c_str(), &data_.intensity, 0.01f, 0.0f, 0.0f, "%.1f");
    }
#endif
}
