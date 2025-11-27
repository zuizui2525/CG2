#include "Input.h"
#include <cassert>
#include <cstring>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

Input::Input() {
    memset(key_, 0, sizeof(key_));
    memset(preKey_, 0, sizeof(preKey_));
    memset(&mouseState_, 0, sizeof(mouseState_));
    memset(&preMouseState_, 0, sizeof(preMouseState_));
}

Input::~Input() {
    if (keyboard_) keyboard_->Unacquire();
    if (mouse_) mouse_->Unacquire();
}

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
    HRESULT hr = DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
        (void**)&directInput_, nullptr);
    assert(SUCCEEDED(hr));

    // ==== キーボード ====
    hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
    assert(SUCCEEDED(hr));

    hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    hr = keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    // ==== マウス ====
    hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
    assert(SUCCEEDED(hr));

    hr = mouse_->SetDataFormat(&c_dfDIMouse2);
    assert(SUCCEEDED(hr));

    hr = mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    assert(SUCCEEDED(hr));
}

void Input::Update() {
    // キーボード
    memcpy(preKey_, key_, sizeof(key_));
    if (keyboard_) {
        keyboard_->Acquire();
        keyboard_->GetDeviceState(sizeof(key_), key_);
    }

    // マウス
    preMouseState_ = mouseState_;
    if (mouse_) {
        mouse_->Acquire();
        // マウスの状態を取得
        mouse_->GetDeviceState(sizeof(mouseState_), &mouseState_);

        // 取得した相対移動量（lX, lY）をアプリケーション座標（mousePos_）に加算
        mousePos_.x += static_cast<float>(mouseState_.lX);
        mousePos_.y += static_cast<float>(mouseState_.lY);
    }
}

// ==== キーボード ====
bool Input::Trigger(BYTE keyCode) const {
    return (key_[keyCode] & 0x80) && !(preKey_[keyCode] & 0x80);
}

bool Input::Press(BYTE keyCode) const {
    return (key_[keyCode] & 0x80);
}

bool Input::Release(BYTE keyCode) const {
    return !(key_[keyCode] & 0x80) && (preKey_[keyCode] & 0x80);
}

// ==== マウス ====
bool Input::MouseTrigger(int button) const {
    return (mouseState_.rgbButtons[button] & 0x80) && !(preMouseState_.rgbButtons[button] & 0x80);
}

bool Input::MousePress(int button) const {
    return (mouseState_.rgbButtons[button] & 0x80);
}

bool Input::MouseRelease(int button) const {
    return !(mouseState_.rgbButtons[button] & 0x80) && (preMouseState_.rgbButtons[button] & 0x80);
}

// マウス座標をVector2で設定
void Input::SetMousePos(const Vector2& pos) {
    mousePos_ = pos;
}
