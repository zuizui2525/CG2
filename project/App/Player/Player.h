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

class MapChipField;

class Player {
private:
    struct CollisionMapInfo {
        bool ceilingCollisionFlag = false;
        bool landingFlag = false;
        bool wallContactFlag = false;
        Vector3 velocity = {};
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

    void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
    void SetPosition(const Vector3& pos) { position_ = pos; }
    const Vector3& GetPosition() const { return position_; }
    
    bool IsAttacking() const {
        return behavior_ == Behavior::kAttack && attackPhase_ == AttackPhase::rush;
    }

    bool IsDead() const { return isDead_; }
    void OnCollisionWithEnemy() { isDead_ = true; }

private:
    void BehaviorRootUpdate();
    void BehaviorAttackUpdate();
    void BehaviorRootInitialize();
    void BehaviorAttackInitialize();

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
    Input* input_ = nullptr;
    std::unique_ptr<ModelObject> model_;
    MapChipField* mapChipField_ = nullptr;

    Vector3 position_ = { 0.4f, 0.4f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 scale_ = { 0.1f, 0.1f, 0.1f };
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };

    static inline const float kAcceleration = 0.005f;
    static inline const float kAttenuation = 0.1f;
    static inline const float kLimitRunSpeed = 0.03f;
    static inline const float kGravityAccleration = 0.005f;
    static inline const float kLimitFallSpeed = 0.15f;
    static inline const float kJumpAcceleration = 0.09f;
    static inline const float kTimeTurn = 0.2f;
    static inline const float kWidth = 0.09f;
    static inline const float kHeight = 0.09f;
    static inline const float kAttenuationLanding = 0.8f;

    LRDirection lrDirection_ = LRDirection::kRight;
    float turnTimer_ = 0.0f;
    bool onGround_ = true;
    Behavior behavior_ = Behavior::kRoot;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    uint32_t attackParameter_ = 0;
    AttackPhase attackPhase_ = AttackPhase::reservoir;

    bool canDoubleJump_ = false;
    bool isWallJumping_ = false;
    float wallJumpTimer_ = 0.0f;

    bool isDead_ = false;
};
