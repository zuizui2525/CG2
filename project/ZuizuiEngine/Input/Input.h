#pragma once
#include <dinput.h>
#include <wrl.h>
#include <Windows.h>
#include "Struct.h" // Vector2を参照するため追加

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

    // マウス座標取得 (float型)
    float GetMouseX() const { return mousePos_.x; }
    float GetMouseY() const { return mousePos_.y; }

    // マウスホイール取得 (float型)
    float GetMouseWheel() const { return static_cast<float>(mouseState_.lZ); }

    // マウス座標をVector2で取得
    Vector2 GetMousePos() const { return mousePos_; }

    // マウス座標設定 (Set関数)
    void SetMouseX(float x) { mousePos_.x = x; }
    void SetMouseY(float y) { mousePos_.y = y; }
    void SetMousePos(const Vector2& pos);

private:
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

    BYTE key_[256]{};     // 現在のキー状態
    BYTE preKey_[256]{};  // 1フレーム前のキー状態

    DIMOUSESTATE2 mouseState_{};     // 現在のマウス状態
    DIMOUSESTATE2 preMouseState_{};  // 1フレーム前のマウス状態

    // アプリケーション側で管理するマウス座標
    Vector2 mousePos_{};
};
