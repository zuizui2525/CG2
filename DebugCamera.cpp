#include "DebugCamera.h"

void DebugCamera::Initialize() {
    projectionMatrix_ = Math::MakePerspectiveFovMatrix(0.45f, 16.0f / 9.0f, 0.1f, 1000.0f); // 仮の値
}

void DebugCamera::Update(BYTE key[], DIMOUSESTATE2 mouseState) {
    POINT center;
    center.x = 640;
    center.y = 360;

    POINT currentPos;
    GetCursorPos(&currentPos);
    // 1フレームだけマウス移動による回転をスキップする
    if (skipNextMouseUpdate_) {
        skipNextMouseUpdate_ = false;

        // カーソルを中央に戻す（差分をリセット）
        ClientToScreen(hwnd_, &center);
        SetCursorPos(center.x, center.y);
        return; // このフレームのカメラ更新はスキップ
    }
    ScreenToClient(hwnd_, &currentPos); // ← hwndはWindowハンドル

    // 差分取得（中央との差）
    int dx = currentPos.x - center.x;
    int dy = currentPos.y - center.y;

    // 回転角度を加算
    rotation_.x += static_cast<float>(dy) * rotateSpeed;
    rotation_.y += static_cast<float>(dx) * rotateSpeed;

    // 角度制限
    rotation_.x = std::clamp(rotation_.x, -1.57f, 1.57f);
    
    // ロール固定（Z軸の傾きを防ぐ）
    rotation_.z = 0.0f;

    // カーソルを中央に戻す
    ClientToScreen(hwnd_, &center);
    SetCursorPos(center.x, center.y);

    // Wキーが押されている場合、前方へ移動
    if (key[DIK_W] & 0x80) {
        translation_.x += forwardVector_.x * moveSpeed_;
        translation_.y += forwardVector_.y * moveSpeed_;
        translation_.z += forwardVector_.z * moveSpeed_;
    }

    // Sキーが押されている場合、後方へ移動
    if (key[DIK_S] & 0x80) {
        translation_.x -= forwardVector_.x * moveSpeed_;
        translation_.y -= forwardVector_.y * moveSpeed_;
        translation_.z -= forwardVector_.z * moveSpeed_;
    }

    // Aキーが押されている場合、左へ移動
    if (key[DIK_A] & 0x80) {
        translation_.x -= rightVector_.x * moveSpeed_;
        translation_.y -= rightVector_.y * moveSpeed_;
        translation_.z -= rightVector_.z * moveSpeed_;
    }

    // Dキーが押されている場合、右へ移動
    if (key[DIK_D] & 0x80) {
        translation_.x += rightVector_.x * moveSpeed_;
        translation_.y += rightVector_.y * moveSpeed_;
        translation_.z += rightVector_.z * moveSpeed_;
    }

    // SPACEキーが押されている場合、上昇
    if (key[DIK_SPACE] & 0x80) {
        translation_.y += upVector_.y * moveSpeed_; // Y軸プラス方向へ移動
    }

    // LSHIFTキーが押されている場合、下降
    if (key[DIK_LSHIFT] & 0x80) {
        translation_.y -= upVector_.y * moveSpeed_; // Y軸マイナス方向へ移動
    }

    // Rキー押されたら初期化
    if (key[DIK_R] & 0x80) {
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
