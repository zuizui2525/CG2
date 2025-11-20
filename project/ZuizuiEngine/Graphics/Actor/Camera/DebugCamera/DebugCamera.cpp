#include "DebugCamera.h"

void DebugCamera::Initialize() {
    projectionMatrix_ = Math::MakePerspectiveFovMatrix(0.45f, 16.0f / 9.0f, 0.1f, 1000.0f); // 仮の値
}

void DebugCamera::Update(Input* input) {
    POINT center;
    center.x = 640;
    center.y = 360;

    // 1フレームだけマウス移動による回転をスキップする
    if (skipNextMouseUpdate_) {
        skipNextMouseUpdate_ = false;
        ClientToScreen(hwnd_, &center);
        SetCursorPos(center.x, center.y);
        return;
    }

    // 右クリック中のみ視点回転
    if (input->MousePress(1)) {
        POINT currentPos;
        GetCursorPos(&currentPos);
        ScreenToClient(hwnd_, &currentPos);

        int dx = currentPos.x - center.x;
        int dy = currentPos.y - center.y;

        rotation_.x += static_cast<float>(dy) * rotateSpeed;
        rotation_.y += static_cast<float>(dx) * rotateSpeed;
        rotation_.x = std::clamp(rotation_.x, -1.57f, 1.57f);
        rotation_.z = 0.0f;
    }

    // 毎フレーム カーソルを中央に戻す（右クリックしていなくても！）
    ClientToScreen(hwnd_, &center);
    SetCursorPos(center.x, center.y);

    // Wキーが押されている場合、前方へ移動
    if (input->Press(DIK_W)) {
        translation_.x += forwardVector_.x * moveSpeed_;
        translation_.y += forwardVector_.y * moveSpeed_;
        translation_.z += forwardVector_.z * moveSpeed_;
    }

    // Sキーが押されている場合、後方へ移動
    if (input->Press(DIK_S)) {
        translation_.x -= forwardVector_.x * moveSpeed_;
        translation_.y -= forwardVector_.y * moveSpeed_;
        translation_.z -= forwardVector_.z * moveSpeed_;
    }

    // Aキーが押されている場合、左へ移動
    if (input->Press(DIK_A)) {
        translation_.x -= rightVector_.x * moveSpeed_;
        translation_.y -= rightVector_.y * moveSpeed_;
        translation_.z -= rightVector_.z * moveSpeed_;
    }

    // Dキーが押されている場合、右へ移動
    if (input->Press(DIK_D)) {
        translation_.x += rightVector_.x * moveSpeed_;
        translation_.y += rightVector_.y * moveSpeed_;
        translation_.z += rightVector_.z * moveSpeed_;
    }

    // SPACEキーが押されている場合、上昇
    if (input->Press(DIK_SPACE)) {
        translation_.y += upVector_.y * moveSpeed_; // Y軸プラス方向へ移動
    }

    // LSHIFTキーが押されている場合、下降
    if (input->Press(DIK_LSHIFT)) {
        translation_.y -= upVector_.y * moveSpeed_; // Y軸マイナス方向へ移動
    }

    // Rキー押されたら初期化
    if (input->Press(DIK_R)) {
        ResetPosition();

        // もし必要ならマウスカーソル位置も中央に戻す
        POINT center = { 640, 360 };
        ClientToScreen(hwnd_, &center);
        SetCursorPos(center.x, center.y);

        // マウス移動の差分リセット用フラグも立てておく
        skipNextMouseUpdate_ = true;
    }

    // カメラの回転行列を先に計算します
    Matrix4x4 rotateXMatrix = Math::MakeRotateXMatrix(rotation_.x);
    Matrix4x4 rotateYMatrix = Math::MakeRotateYMatrix(rotation_.y);

    // 回転の適用順序は、X -> Y -> Z が一般的ですが、ジンバルロックに注意
    Matrix4x4 rotateMatrix = Math::Multiply(rotateXMatrix, rotateYMatrix);

    // カメラの向きを更新
    forwardVector_ = Math::Normalize(Math::TransformNormal({ 0, 0, 1 }, rotateMatrix));
    // rightVector_はforwardとワールドアップ（0,1,0）の外積
    rightVector_ = Math::Normalize(Math::Cross({ 0.0f, 1.0f, 0.0f }, forwardVector_));
    // upVector_はrightとforwardの外積（こっちも逆になるかも。こっちは普通に）
    upVector_ = Math::Cross(forwardVector_, rightVector_);

    // カメラの位置と回転からビュー行列を作成（傾かない版）
    viewMatrix_ = Math::MakeLookAtMatrix(
        translation_,
        Math::Add(translation_,forwardVector_),
        { 0.0f, 1.0f, 0.0f }
    );
}

void DebugCamera::HideCursor() {
    if (!isCursorHidden_) {
        while (ShowCursor(FALSE) >= 0); // 完全に非表示に
        SetCursor(nullptr);
        isCursorHidden_ = true;
    }
}

void DebugCamera::ShowCursorBack() {
    if (isCursorHidden_) {
        while (ShowCursor(TRUE) < 0); // 完全に表示に
        isCursorHidden_ = false;
    }
}

void DebugCamera::ResetPosition() {
    translation_ = { 0.0f, 0.0f, -10.0f };  // 好きな初期位置に設定
    rotation_ = { 0.0f, 0.0f, 0.0f };     // 回転リセット
}
