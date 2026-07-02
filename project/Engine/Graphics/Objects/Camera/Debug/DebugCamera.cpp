#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"
#include <algorithm>
#include "imgui.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Zuizui.h"

void DebugCamera::Initialize() {
    BaseCamera::Initialize();
    hwnd_ = EngineResource::GetEngine()->GetWindow()->GetHWND();
    assert(hwnd_ != nullptr);
}

void DebugCamera::Update(Input* input) {
#ifdef _USEIMGUI
    if (!hwnd_ || !isActive_) return;

    // Ctrl キー（左または右）が押されているときのみ、カメラ操作を有効にする
    bool isOperating = input->Press(DIK_LCONTROL) || input->Press(DIK_RCONTROL);

    if (isOperating) {
        // --- 1. カーソル制御と中央固定 ---
        int centerX = WindowApp::kClientWidth / 2;
        int centerY = WindowApp::kClientHeight / 2;
        POINT center = { centerX, centerY };

        // 操作中はカーソルを隠す
        SetCursorVisible(false);

        POINT currentPos;
        GetCursorPos(&currentPos);
        ScreenToClient(hwnd_, &currentPos);

        // 中心からの移動量を取得
        int dx = currentPos.x - center.x;
        int dy = currentPos.y - center.y;

        // マウスを中央に戻す
        ClientToScreen(hwnd_, &center);
        SetCursorPos(center.x, center.y);

        // --- 2. 回転処理 ---
        transform_.rotate.x += static_cast<float>(dy) * rotateSpeed_;
        transform_.rotate.y += static_cast<float>(dx) * rotateSpeed_;

        // 垂直方向の回転制限
        transform_.rotate.x = std::clamp(transform_.rotate.x, -1.57f, 1.57f);

        // --- 3. 移動処理 (WASD) ---
        Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
        Vector3 forward = Math::TransformNormal({ 0, 0, 1 }, rotateMatrix);
        Vector3 right = Math::TransformNormal({ 1, 0, 0 }, rotateMatrix);
        Vector3 up = { 0, 1, 0 };

        Vector3 move = { 0, 0, 0 };
        if (input->Press(DIK_W)) move = move + forward;
        if (input->Press(DIK_S)) move = move - forward;
        if (input->Press(DIK_D)) move = move + right;
        if (input->Press(DIK_A)) move = move - right;
        if (input->Press(DIK_SPACE)) move = move + up;
        if (input->Press(DIK_LSHIFT)) move = move - up;

        if (Math::Length(move) > 0) {
            move = Math::Normalize(move) * moveSpeed_;
            transform_.translate = transform_.translate + move;
        }
    } else {
        // キーが離されているときはカーソルを表示する
        SetCursorVisible(true);
    }

    BaseCamera::Update();
#else
    (void)input;
#endif
}

void DebugCamera::SetCursorVisible(bool isVisible) {
    if (isVisible && !isCursorVisible_) {
        while (ShowCursor(TRUE) < 0);
        isCursorVisible_ = true;
    } else if (!isVisible && isCursorVisible_) {
        while (ShowCursor(FALSE) >= 0);
        isCursorVisible_ = false;
    }
}

void DebugCamera::SetActive(bool active) {
    isActive_ = active;
    // 非アクティブになる瞬間にカーソルを表示させる
    if (!isActive_) {
        DebugCamera::SetCursorVisible(true);
    }
}

