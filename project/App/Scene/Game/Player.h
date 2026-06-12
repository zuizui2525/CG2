#pragma once
#include "App/Scene/Game/Shooter.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"

/**
 * @brief プレイヤーキャラクタークラス
 */
class Player : public Shooter {
public:
    Player();
    ~Player() override = default;

    void Initialize() override;
    void Update() override; // 継承元に合わせるため引数なし、Update内でInputResource等から取得
    void UpdateInput(Input* input); // インプットを明示的に受ける更新メソッド
    void Draw() override;

    // 衝突判定用インターフェース実装
    Vector3 GetPosition() const override { return cube_->GetPosition(); }
    Vector3 GetSize() const override { return cube_->GetSize(); }

private:
    // マジックナンバー排除のための定数
    static inline const Vector3 kInitialPosition = { 0.0f, 0.0f, -6.0f };   // 初期位置
    static inline const Vector3 kPlayerScale = { 1.0f, 1.0f, 1.0f };        // サイズ
    static inline const Vector3 kBulletVelocity = { 0.0f, 0.0f, 0.25f };    // 弾の初速度
    static inline const float kSpeed = 0.1f;                                // 移動速度
    static inline const float kMoveLimitX = 8.0f;                           // 左右移動限界
    static inline const std::string kTextureKey = "white";                  // テクスチャ
    static inline const std::string kEnvMapKey = "";                        // 環境マップ
    static inline const int kMoveLeftKey = DIK_A;                           // 左移動キー
    static inline const int kMoveRightKey = DIK_D;                          // 右移動キー
    static inline const int kShotKey = DIK_SPACE;                           // 射撃キー

private:
    std::unique_ptr<CubeObject> cube_;
};
