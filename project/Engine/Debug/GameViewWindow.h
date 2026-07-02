#pragma once
#include "Engine/Math/MathStructs.h"

#ifdef _USEIMGUI
#include "Engine/Debug/PopAnimation.h"
#endif

class GameViewWindow {
public:
#ifdef _USEIMGUI
    GameViewWindow();
    ~GameViewWindow() = default;

    void Draw(bool* show, bool* isVisible);

    // 左クリック一時停止の有効・無効を取得・設定
    bool IsClickPauseEnabled() const { return isClickPauseEnabled_; }
    void SetClickPauseEnabled(bool enable) { isClickPauseEnabled_ = enable; }

    // ギズモ操作モードの取得・設定 (7: TRANSLATE, 120: ROTATE, 896: SCALE)
    int GetGizmoOperation() const { return gizmoOperation_; }
    void SetGizmoOperation(int op) { gizmoOperation_ = op; }

    // ギズモ表示・非表示の取得・設定
    bool IsShowGizmo() const { return showGizmo_; }
    void SetShowGizmo(bool show) { showGizmo_ = show; }
#endif

    // マウスが有効なゲーム画面領域上にあるか
    static bool IsMouseOnGameView();
    // ゲーム画面のサイズ
    static Vector2 GetGameViewSize();
    // ゲーム画面上のローカルマウス座標
    static Vector2 GetMousePosition();
    // ゲーム画面の左上座標
    static Vector2 GetGameViewPosMin();

private:
#ifdef _USEIMGUI
    PopAnimation popAnim_;
    bool wasPaused_ = false;
    bool isClickPauseEnabled_ = true; // 左クリックによる一時停止を有効にするかどうかのフラグ
    bool showGizmo_ = true;           // ギズモを描画・利用するかどうかのフラグ
    int gizmoOperation_ = 7;          // ギズモの操作モード (初期値: TRANSLATE = 7)
    static inline bool sIsMouseOnGameView_ = false;
    static inline Vector2 sGameViewSize_ = { 0.0f, 0.0f };
    static inline Vector2 sGameViewPosMin_ = { 0.0f, 0.0f };
#endif
};
