#include "DirectionalLight.h"
#include "../../Matrix/Matrix.h"

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

void DirectionalLightObject::ImGuiControl() {
    ImGui::Text("DirectionalLight");
    ImGui::ColorEdit4("Color", &lightData_->color.x);
    ImGui::DragFloat3("Direction", &lightData_->direction.x, 0.01f);
    ImGui::DragFloat("Intensity", &lightData_->intensity, 0.01f);
}
