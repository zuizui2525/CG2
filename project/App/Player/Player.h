#pragma once
#include "ModelObject.h"
#include "Camera.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include "Input.h"
#include <memory>
#include <vector>

// 前方宣言
class MapChipField;

class Player {
private:
    // マップとの当たり判定情報
    struct CollisionMapInfo {
        bool ceilingCollisionFlag = false; // 天井衝突フラグ
        bool landingFlag = false;          // 着地フラグ
        bool wallContactFlag = false;      // 壁接触フラグ
        Vector3 velocity;                  // 移動量
    };

    enum class LRDirection { kRight, kLeft };
    enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };
    enum class Behavior { kUnknown, kRoot, kAttack };
    enum class AttackPhase { reservoir, rush, lingeringSound };

public:
    Player();
    ~Player();

    void Initialize(DxCommon* dxCommon, TextureManager* textureManager, Input* input);
    void Update(Camera* camera);
    void Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight);

    // Setter
    void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
    void SetPosition(const Vector3& pos) { position_ = pos; }

    // Getter
    const Vector3& GetPosition() const { return position_; }

private:
    // 行動更新
    void BehaviorRootUpdate();
    void BehaviorAttackUpdate();
    void BehaviorRootInitialize();
    void BehaviorAttackInitialize();

    // 移動・衝突判定
    void HandleMoveInput();
    void MapCollisionDetection(CollisionMapInfo& info);
    void MapCollisionDetectionUp(CollisionMapInfo& info);
    void MapCollisionDetectionDown(CollisionMapInfo& info);
    void MapCollisionDetectionLeft(CollisionMapInfo& info);
    void MapCollisionDetectionRight(CollisionMapInfo& info);
    Vector3 CornerPosition(const Vector3& center, Corner corner);

    void touchingTheCeiling(const CollisionMapInfo& info);
    void SwitchingInstallationStatus(const CollisionMapInfo& info);
    void contactWithAWall(const CollisionMapInfo& info);

private:
    // 基盤
    Input* input_ = nullptr;
    std::unique_ptr<ModelObject> model_;
    MapChipField* mapChipField_ = nullptr;

    // トランスフォーム
    Vector3 position_ = { 0.2f, 0.2f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 scale_ = { 0.1f, 0.1f, 0.1f };
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };

    // 定数パラメータ
    static inline const float kAcceleration = 0.01f;
    static inline const float kAttenuation = 0.05f;
    static inline const float kLimitRunSpeed = 0.08f;
    static inline const float kGravityAccleration = 0.005f;
    static inline const float kLimitFallSpeed = 0.15f;
    static inline const float kJumpAcceleration = 0.12f;
    static inline const float kTimeTurn = 0.2f;
    static inline const float kWidth = 0.09f;  // 当たり判定の幅
    static inline const float kHeight = 0.09f; // 当たり判定の高さ
    static inline const float kAttenuationLanding = 0.8f;
    static inline const float kAttenuationWall = 0.8f;

    // 状態管理
    LRDirection lrDirection_ = LRDirection::kRight;
    float turnFirstRotationY_ = 0.0f;
    float turnTimer_ = 0.0f;
    bool onGround_ = true;
    Behavior behavior_ = Behavior::kRoot;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    // 攻撃・壁ジャンプ関連
    uint32_t attackParameter_ = 0;
    AttackPhase attackPhase_ = AttackPhase::reservoir;
    bool isWallJumping_ = false;
    float wallJumpTimer_ = 0.0f;
};
