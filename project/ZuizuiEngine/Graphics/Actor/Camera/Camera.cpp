#include "Camera.h"

void Camera::Initialize() {
    transform_ = { {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-7.0f} };
    debugCamera_.Initialize();

    viewMatrix2D_ = Math::MakeIdentity();
    projectionMatrix2D_ = Math::MakeOrthographicMatrix(
        0.0f, 0.0f,
        static_cast<float>(WindowApp::kClientWidth),
        static_cast<float>(WindowApp::kClientHeight),
        0.0f, 100.0f
    );

    Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(
        transform_.scale, transform_.rotate, transform_.translate
    );
    viewMatrix3D_ = Math::Inverse(cameraMatrix);
    projectionMatrix3D_ = Math::MakePerspectiveFovMatrix(
        0.45f,
        static_cast<float>(WindowApp::kClientWidth) / static_cast<float>(WindowApp::kClientHeight),
        0.1f, 100.0f
    );
}

void Camera::Update(Input* input) {
    // TABで切り替え
    if (input->Trigger(DIK_TAB)) {
        useDebugCamera_ = !useDebugCamera_;
    }

    Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(
        transform_.scale, transform_.rotate, transform_.translate
    );

    if (useDebugCamera_) {
        if (!wasDebugCameraLastFrame_) {
            debugCamera_.skipNextMouseUpdate_ = true;
        }
        debugCamera_.HideCursor();
        debugCamera_.Update(input);
        viewMatrix3D_ = debugCamera_.GetViewMatrix();
        projectionMatrix3D_ = debugCamera_.GetProjectionMatrix();
    } else {
        debugCamera_.ShowCursorBack();
        viewMatrix3D_ = Math::Inverse(cameraMatrix);
        projectionMatrix3D_ = Math::MakePerspectiveFovMatrix(
            0.45f,
            static_cast<float>(WindowApp::kClientWidth) / static_cast<float>(WindowApp::kClientHeight),
            0.1f, 100.0f
        );
    }

    wasDebugCameraLastFrame_ = useDebugCamera_;
}

void Camera::ImGuiControl() {
#ifdef _DEBUG
    ImGui::Text("Camera");
    ImGui::DragFloat3("Scale(Camera)", &transform_.scale.x, 0.01f);
    ImGui::DragFloat3("Rotate(Camera)", &transform_.rotate.x, 0.01f);
    ImGui::DragFloat3("Translate(Camera)", &transform_.translate.x, 0.01f);
    ImGui::Separator();
    ImGui::Text("DebugCamera");
    if (useDebugCamera_) {
        ImGui::Text("Running (TAB to disable)");
    } else {
        ImGui::Text("Disabled (TAB to enable)");
    }
    ImGui::Separator();
#endif
}
