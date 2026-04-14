#include "BaseCamera.h"
#include "WindowApp.h"
#include "ImGuiManager.h"

void BaseCamera::Initialize() {
    transform_ = { {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-20.0f} };

    // 射影行列の初期化
    projectionMatrix_ = Math::MakePerspectiveFovMatrix(
        0.45f,
        static_cast<float>(WindowApp::kClientWidth) / static_cast<float>(WindowApp::kClientHeight),
        0.1f, 1000.0f
    );
}

void BaseCamera::Update() {
    if (useTarget_) {
        // 注視点モード
        viewMatrix_ = Math::MakeLookAtMatrix(transform_.translate, target_, { 0.0f, 1.0f, 0.0f });
    } else {
        // 回転モード
        Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
        viewMatrix_ = Math::Inverse(cameraMatrix);
    }
}

void BaseCamera::ImGuiControl(const std::string& name) {
#ifdef _USEIMGUI
    // List側のチェックボックス
    ImGui::Checkbox(("Edit##" + name).c_str(), &isWindowOpen_);

    if (isWindowOpen_) {
        // ウィンドウ表示
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            std::string tag = "##" + name;
            if (ImGui::CollapsingHeader(("Transform" + tag).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3(("Pos" + tag).c_str(), &transform_.translate.x, 0.1f, 0, 0, "%.1f");
                ImGui::DragFloat3(("Rot" + tag).c_str(), &transform_.rotate.x, 0.1f, 0, 0, "%.1f");
            }
        }
        ImGui::End(); // 一旦閉じる（派生クラス側で追記できるようにするため）
    }
#endif
}
