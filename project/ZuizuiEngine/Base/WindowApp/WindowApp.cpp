#include "WindowApp.h"
#include "imgui_impl_win32.h"
#include <cassert>
#pragma comment(lib, "winmm.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WindowApp::WindowApp() {}
WindowApp::~WindowApp() {
    if (hwnd_) {
        CloseWindow(hwnd_);
    }
}

bool WindowApp::Initialize(const wchar_t* title) {
    timeBeginPeriod(1);
    wc_.lpfnWndProc = WindowProc;
    wc_.lpszClassName = L"MyWindowClass";
    wc_.hInstance = GetModuleHandle(nullptr);
    wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc_);

    AdjustWindowRect(&wrc_, WS_OVERLAPPEDWINDOW, false);

    hwnd_ = CreateWindow(
        wc_.lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc_.right - wrc_.left,
        wrc_.bottom - wrc_.top,
        nullptr,
        nullptr,
        wc_.hInstance,
        nullptr
    );

    assert(hwnd_ != nullptr);
    return hwnd_ != nullptr;
}

void WindowApp::Show() {
    ShowWindow(hwnd_, SW_SHOW);
}

bool WindowApp::ProcessMessage() {
    MSG msg{};
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

LRESULT CALLBACK WindowApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // ImGui用の処理を先に通す（ここ重要！）
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

