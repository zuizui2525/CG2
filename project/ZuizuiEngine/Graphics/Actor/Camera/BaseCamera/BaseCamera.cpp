#include "BaseCamera.h"
#include "WindowApp.h"

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
