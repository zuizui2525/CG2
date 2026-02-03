#pragma once
#include "BaseCamera.h"
#include "Input.h"
#include "WindowApp.h"

class DebugCamera : public BaseCamera {
public:
    void Initialize() override;

    // デバッグカメラ特有の更新（Inputを受け取る）
    void Update(Input* input);
    void Update() override {}

    // ImGuiでのガイド表示
    void ImGuiControl();

    void SetHwnd(HWND hwnd) { hwnd_ = hwnd; }

    // カーソル表示・非表示を外部から強制的に変える（カメラ切り替え時用）
    void SetCursorVisible(bool isVisible);
    void SetActive(bool active);
private:
    float moveSpeed_ = 0.2f;
    const float rotateSpeed_ = 0.002f;
    HWND hwnd_ = nullptr;

    bool isActive_ = false;
    bool isCursorVisible_ = true;
};
