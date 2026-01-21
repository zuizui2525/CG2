#include "PointLight.h"
#include "imgui.h" // ImGuiを使う場合

void PointLightObject::Initialize(ID3D12Device* device) {
    // ステップ1で作った PointLight 構造体のサイズでリソース作成
    resource_ = CreateBufferResource(device, sizeof(PointLight));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

    // 初期値の設定
    lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->position = { 0.0f, 2.0f, 0.0f }; // 少し浮かせた位置
    lightData_->intensity = 1.0f;
    lightData_->radius = 10.0f; // 10m先まで届く
    lightData_->decay = 1.0f;  // 減衰率
}

void PointLightObject::Update() {
    // 点光源は方向の正規化などが不要なので、今は空でもOK
}

void PointLightObject::ImGuiControl() {
    ImGui::Begin("Light Control"); // 既存のライトウィンドウがあればそこにまとめる
    if (ImGui::CollapsingHeader("PointLight")) {
        ImGui::ColorEdit4("Color", &lightData_->color.x);
        ImGui::DragFloat3("Position", &lightData_->position.x, 0.1f);
        ImGui::DragFloat("Intensity", &lightData_->intensity, 0.01f);
        ImGui::DragFloat("Radius", &lightData_->radius, 0.1f);
        ImGui::DragFloat("Decay", &lightData_->decay, 0.01f);
    }
    ImGui::End();
}
