#pragma once
#include <Windows.h>
#include <string>

class WindowApp {
public:
    // 定数
    static const int kClientWidth = 1280;
    static const int kClientHeight = 720;

    // コンストラクタ・デストラクタ
    WindowApp();
    ~WindowApp();

    // 初期化・表示
    bool Initialize(const wchar_t* title);
    void Show();

    // メッセージループ
    bool ProcessMessage();

    // getter
    HWND GetHWND() const { return hwnd_; }
    HINSTANCE GetInstance() const { return wc_.hInstance; }
    RECT GetClientRect() const { return wrc_; }

private:
    // ウィンドウプロシージャ（静的）
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    // 内部データ
    HWND hwnd_ = nullptr;
    RECT wrc_ = { 0, 0, kClientWidth, kClientHeight };
    WNDCLASS wc_ = {};
};