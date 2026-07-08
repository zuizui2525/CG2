#pragma once
#include <random>
#include "App/Scene/Game/Shooter.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"
#include "App/Scene/Game/PartCollider.h"

class Player;

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
    void SetPosition(const Vector3& pos) { cube_->SetPosition(pos); }
    void SetSize(const Vector3& size) { cube_->SetSize(size); }
    CubeObject* GetCube() const { return cube_.get(); }

    // コライダーおよびターゲット関連
    PartCollider* GetBodyCollider() const { return bodyCollider_.get(); }
    PartCollider* GetHeadCollider() const { return headCollider_.get(); }
    void SetTargetPlayer(Player* player) { targetPlayer_ = player; }

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
    static inline const Vector4 kEnemyColor = { 1.0f, 0.1f, 0.1f, 1.0f };    // 敵のカラー（赤）
    static inline const std::string kBulletEffectName = "PurpleFire";       // 弾エフェクト名（紫の炎）

    // 頭部およびコライダーのローカル定数
    static inline const Vector3 kHeadLocalPos = { 0.0f, 1.0f, 0.0f };
    static inline const Vector3 kHeadLocalScale = { 0.6f, 0.6f, 0.6f };
    static inline const Vector4 kHeadColor = { 1.0f, 0.8f, 0.8f, 1.0f };
    static inline const Vector3 kBodyColliderSize = { 1.0f, 1.0f, 1.0f };
    static inline const Vector3 kHeadColliderSize = { 0.6f, 0.6f, 0.6f };

private:
    std::unique_ptr<CubeObject> cube_;                 // 胴体モデル
    std::unique_ptr<CubeObject> headCube_;             // 頭部モデル
    std::unique_ptr<PartCollider> bodyCollider_;       // 胴体コライダー
    std::unique_ptr<PartCollider> headCollider_;       // 頭部コライダー
    Player* targetPlayer_ = nullptr;                   // 自機へのポインタ

    // 移動および射撃の制御用タイマーと状態
    float moveDirection_ = 1.0f; // 1.0f または -1.0f
    int moveTimer_ = 0;
    int shotTimer_ = 0;

    // 乱数生成器
    std::mt19937 randomEngine_;
};
