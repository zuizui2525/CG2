#include "Input.h"
#include "Zuizui.h"
#include "BaseResource.h"
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

void Input::Initialize() {
    // 1. EngineResourceから必要な情報を取得
    auto engine = EngineResource::GetEngine();
    assert(engine != nullptr);

    // 2. DirectInput8Create (ComPtrの扱いを修正)
    HRESULT hr = DirectInput8Create(
        engine->GetWindow()->GetInstance(),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)directInput_.GetAddressOf(), // &ではなくGetAddressOf()を使う
        nullptr);
    assert(SUCCEEDED(hr));

    // 3. キーボードデバイスの作成
    hr = directInput_->CreateDevice(GUID_SysKeyboard, keyboard_.GetAddressOf(), NULL);
    assert(SUCCEEDED(hr));

    hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    // engine経由でHWNDを取得
    hr = keyboard_->SetCooperativeLevel(
        engine->GetWindow()->GetHWND(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    // 4. マウスデバイスの作成
    hr = directInput_->CreateDevice(GUID_SysMouse, mouse_.GetAddressOf(), NULL);
    assert(SUCCEEDED(hr));

    hr = mouse_->SetDataFormat(&c_dfDIMouse2);
    assert(SUCCEEDED(hr));

    hr = mouse_->SetCooperativeLevel(
        engine->GetWindow()->GetHWND(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
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
