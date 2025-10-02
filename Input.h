#pragma once
#include <dinput.h>
#include <wrl.h>
#include <Windows.h>

class Input {
public:
    Input();
    ~Input();

    // 初期化（キーボード + マウス）
    void Initialize(HINSTANCE hInstance, HWND hwnd);

    // 毎フレーム更新
    void Update();

    // ==== キーボード ====
    bool Trigger(BYTE key) const;   // 押した瞬間
    bool Press(BYTE key) const;     // 押している間
    bool Release(BYTE key) const;   // 離した瞬間

    // ==== マウス ====
    // button: 0=左, 1=右, 2=中
    bool MouseTrigger(int button) const;
    bool MousePress(int button) const;
    bool MouseRelease(int button) const;

    // マウス座標取得
    LONG GetMouseX() const { return mouseState_.lX; }
    LONG GetMouseY() const { return mouseState_.lY; }
    LONG GetMouseZ() const { return mouseState_.lZ; } // ホイール

private:
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

    BYTE key_[256]{};     // 現在のキー状態
    BYTE preKey_[256]{};  // 1フレーム前のキー状態

    DIMOUSESTATE2 mouseState_{};     // 現在のマウス状態
    DIMOUSESTATE2 preMouseState_{};  // 1フレーム前のマウス状態
};