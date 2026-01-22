#include "Camera.h"

void Camera::Initialize(ID3D12Device* device, Input* input) {
    input_ = input;
    transform_ = { {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-20.0f} };
    debugCamera_.Initialize();

    viewMatrix2D_ = Math::MakeIdentity();
    projectionMatrix2D_ = Math::MakeOrthographicMatrix(
        0.0f, 0.0f,
        static_cast<float>(WindowApp::kClientWidth),
        static_cast<float>(WindowApp::kClientHeight),
        0.0f, 100.0f
    );

    cameraMatrix_ = Math::MakeAffineMatrix(
        transform_.scale, transform_.rotate, transform_.translate
    );
    viewMatrix3D_ = Math::Inverse(cameraMatrix_);
    projectionMatrix3D_ = Math::MakePerspectiveFovMatrix(
        0.45f,
        static_cast<float>(WindowApp::kClientWidth) / static_cast<float>(WindowApp::kClientHeight),
        0.1f, 100.0f
    );

    resource_ = CreateBufferResource(device, (sizeof(CameraForGPU)));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
}

void Camera::Update() {
    if (!input_) return;
    // TABで切り替え
    if (input_->Trigger(DIK_TAB)) {
        useDebugCamera_ = !useDebugCamera_;
    }

    cameraMatrix_ = Math::MakeAffineMatrix(
        transform_.scale, transform_.rotate, transform_.translate
    );

    if (useDebugCamera_) {
        if (!wasDebugCameraLastFrame_) {
            debugCamera_.skipNextMouseUpdate_ = true;
        }
        debugCamera_.HideCursor();
        debugCamera_.Update(input_);
        viewMatrix3D_ = debugCamera_.GetViewMatrix();
        projectionMatrix3D_ = debugCamera_.GetProjectionMatrix();
    } else {
        debugCamera_.ShowCursorBack();
        viewMatrix3D_ = Math::Inverse(cameraMatrix_);
        projectionMatrix3D_ = Math::MakePerspectiveFovMatrix(
            0.45f,
            static_cast<float>(WindowApp::kClientWidth) / static_cast<float>(WindowApp::kClientHeight),
            0.1f, 100.0f
        );
    }

    wasDebugCameraLastFrame_ = useDebugCamera_;

    if (data_) {
        if (useDebugCamera_) {
           data_->worldPosition = debugCamera_.GetPosition();
        } else {
            data_->worldPosition = transform_.translate;
        }
    }
}

void Camera::ImGuiControl(const std::string& name) {
#ifdef _DEBUG
    // 管理用ウィンドウにチェックボックスを表示
    ImGui::Begin("Camera List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        // 個別制御ウィンドウ
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            std::string label = "##" + name;

            // --- カメラの状態表示 ---
            if (useDebugCamera_) {
                ImGui::TextColored({ 1.0f, 1.0f, 0.0f, 1.0f }, "Status: Debug Camera Active (TAB to switch)");
            } else {
                ImGui::Text("Status: Standard Camera Active (TAB to switch)");
            }
            ImGui::Separator();

            // --- 通常カメラのTransform設定 ---
            if (ImGui::CollapsingHeader(("Transform" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3(("Scale" + label).c_str(), &transform_.scale.x, 0.01f);
                ImGui::DragFloat3(("Rotate" + label).c_str(), &transform_.rotate.x, 0.01f);
                ImGui::DragFloat3(("Translate" + label).c_str(), &transform_.translate.x, 0.1f);
            }

            // --- 追加情報 (デバッグ用) ---
            if (ImGui::CollapsingHeader(("Matrix Info" + label).c_str())) {
                if (useDebugCamera_) {
                    Vector3 pos = debugCamera_.GetPosition();
                    ImGui::Text("Debug Pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                } else {
                    ImGui::Text("Standard Pos: %.2f, %.2f, %.2f", transform_.translate.x, transform_.translate.y, transform_.translate.z);
                }
            }
        }
        ImGui::End();
    }
#endif
}
