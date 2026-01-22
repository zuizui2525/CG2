#include "SpotLight.h"
#include "imgui.h"
#include "Matrix.h"
#include <cmath>

void SpotLightObject::Initialize(ID3D12Device* device) {
    // 定数バッファリソースの作成（Function.hの関数を使用）
    resource_ = CreateBufferResource(device, sizeof(SpotLight));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

    // パラメータの初期化
    lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->position = { 0.0f, 3.0f, 0.0f };
    lightData_->intensity = 1.0f;
    lightData_->direction = { 0.0f, -1.0f, 0.0f }; // 初期方向は真下
    lightData_->distance = 10.0f;
    lightData_->decay = 1.0f;

    inputAngle_ = 45.0f;
    inputFalloffStart_ = 30.0f;

    Update();
}

void SpotLightObject::Update() {
    // 方向ベクトルの正規化
    lightData_->direction = Math::Normalize(lightData_->direction);

    // 度数法からラジアンへ変換 (radian = degree * PI / 180)
    const float PI = 3.1415926535f;
    float radAngle = inputAngle_ * (PI / 180.0f);
    float radFalloff = inputFalloffStart_ * (PI / 180.0f);

    // シェーダーでの計算簡略化のため、cos値を算出してバッファに書き込む
    lightData_->cosAngle = std::cos(radAngle);
    lightData_->cosFalloffStart = std::cos(radFalloff);
}

void SpotLightObject::ImGuiControl(const std::string& name) {
    ImGui::Begin("Light List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            std::string label = "##" + name;

            if (ImGui::CollapsingHeader(("SpotLight Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4(("Color" + label).c_str(), &lightData_->color.x);
                ImGui::DragFloat3(("Position" + label).c_str(), &lightData_->position.x, 0.1f);
                ImGui::DragFloat3(("Direction" + label).c_str(), &lightData_->direction.x, 0.01f);
                ImGui::DragFloat(("Intensity" + label).c_str(), &lightData_->intensity, 0.01f);
                ImGui::DragFloat(("Distance" + label).c_str(), &lightData_->distance, 0.1f);
                ImGui::DragFloat(("Decay" + label).c_str(), &lightData_->decay, 0.01f);

                // 角度設定
                if (ImGui::DragFloat(("Angle" + label).c_str(), &inputAngle_, 0.5f, 0.0f, 90.0f)) {
                    if (inputAngle_ < inputFalloffStart_) inputFalloffStart_ = inputAngle_;
                }
                if (ImGui::DragFloat(("FalloffStart" + label).c_str(), &inputFalloffStart_, 0.5f, 0.0f, 90.0f)) {
                    if (inputFalloffStart_ > inputAngle_) inputAngle_ = inputFalloffStart_;
                }
            }
        }
        ImGui::End();
    }
}
