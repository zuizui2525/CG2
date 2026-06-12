#pragma once
#include <random>
#include "App/Scene/Game/Shooter.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"

/**
 * @brief 敵キャラクタークラス
 */
class Enemy : public Shooter {
public:
    Enemy();
    ~Enemy() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;

    // 衝突判定用インターフェース実装
    Vector3 GetPosition() const override { return cube_->GetPosition(); }
    Vector3 GetSize() const override { return cube_->GetSize(); }

private:
    // マジックナンバー排除のための定数
    static inline const Vector3 kInitialPosition = { 0.0f, 0.0f, 6.0f };    // 初期位置
    static inline const Vector3 kEnemyScale = { 1.0f, 1.0f, 1.0f };         // サイズ
    static inline const Vector3 kBulletVelocity = { 0.0f, 0.0f, -0.15f };   // 弾の初速度
    static inline const float kSpeed = 0.05f;                               // 移動速度
    static inline const float kMoveLimitX = 8.0f;                           // 左右移動限界
    static inline const std::string kTextureKey = "white";                  // テクスチャ
    static inline const std::string kEnvMapKey = "";                        // 環境マップ
    static inline const int kMinMoveTime = 60;                              // 移動方向転換の最小フレーム
    static inline const int kMaxMoveTime = 180;                             // 移動方向転換の最大フレーム
    static inline const int kMinShotTime = 60;                              // 射撃間隔の最小フレーム
    static inline const int kMaxShotTime = 150;                             // 射撃間隔の最大フレーム

private:
    std::unique_ptr<CubeObject> cube_;

    // 移動および射撃の制御用タイマーと状態
    float moveDirection_ = 1.0f; // 1.0f または -1.0f
    int moveTimer_ = 0;
    int shotTimer_ = 0;

    // 乱数生成器
    std::mt19937 randomEngine_;
};
