#include "DebugCamera.h"
#include <algorithm>
#include "imgui.h"
#include "BaseResource.h"
#include "Zuizui.h"

void DebugCamera::Initialize() {
    BaseCamera::Initialize();
    hwnd_ = EngineResource::GetEngine()->GetWindow()->GetHWND();
    assert(hwnd_ != nullptr);
}

void DebugCamera::Update(Input* input) {
    if (!hwnd_ || !isActive_) return;

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

    // --- 2. 回転処理 (右クリック不要) ---
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

    BaseCamera::Update();
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

void DebugCamera::ImGuiControl(const std::string& name) {
#ifdef _USEIMGUI
    // 1. まず親クラスの共通 UI（座標・回転・Editチェックボックス）を表示
    BaseCamera::ImGuiControl(name);

    // 2. ウィンドウが開いていて、かつアクティブな時だけ操作ガイドを出す
    if (isWindowOpen_) {
        // BaseCamera側で Begin が呼ばれているので、同じ名前で Begin すれば中身を追記できる
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {

            std::string tag = "##" + name;

            // 操作設定セクション
            if (ImGui::CollapsingHeader(("Controller Settings" + tag).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                // 移動速度もライトの強さ(Intensity)のように %.1f で調整可能にする
                ImGui::DragFloat(("Move Speed" + tag).c_str(), &moveSpeed_, 0.01f, 0.01f, 2.0f, "%.2f");

                ImGui::Separator();
                ImGui::TextColored({ 1.0f, 1.0f, 0.0f, 1.0f }, "Controls:");
                ImGui::BulletText("W/A/S/D : Move");
                ImGui::BulletText("Space/LShift : Up/Down");
                ImGui::BulletText("Mouse : Rotate (Auto-lock)");
                ImGui::TextDisabled("(Press TAB to release mouse/switch camera)");
            }
        }
        ImGui::End();
    }
#endif
}
