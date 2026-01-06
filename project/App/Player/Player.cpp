#include "Player.h"
#include "MapChipField.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Player::Player() {}
Player::~Player() {}

void Player::Initialize(DxCommon* dxCommon, TextureManager* textureManager, Input* input) {
    input_ = input;
    // モデル生成
    model_ = std::make_unique<ModelObject>(dxCommon->GetDevice(), "resources/AL/player/player.obj", position_);
    rotation_.y = M_PI / 2.0f;

    // テクスチャロード
    textureManager->LoadTexture("player", model_->GetModelData()->material.textureFilePath);
}

void Player::Update(Camera* camera) {
    // 振る舞いリクエスト処理
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;
        if (behavior_ == Behavior::kRoot) BehaviorRootInitialize();
        else if (behavior_ == Behavior::kAttack) BehaviorAttackInitialize();
        behaviorRequest_ = Behavior::kUnknown;
    }

    // 状態別更新
    if (behavior_ == Behavior::kAttack) {
        BehaviorAttackUpdate();
    } else {
        BehaviorRootUpdate();
    }

    // 旋回タイマー更新
    if (turnTimer_ > 0.0f) {
        turnTimer_ -= 1.0f / 60.0f;
        if (turnTimer_ <= 0.0f) {
            turnTimer_ = 0.0f;
            rotation_.y = (lrDirection_ == LRDirection::kRight) ? M_PI / 2.0f : M_PI * 3.0f / 2.0f;
        } else {
            float destRotY = (lrDirection_ == LRDirection::kRight) ? M_PI / 2.0f : M_PI * 3.0f / 2.0f;
            rotation_.y += (destRotY - rotation_.y) * 0.2f;
        }
    }

    // モデルへの反映
    model_->SetPosition(position_);
    model_->SetRotate(rotation_);
    model_->SetScale(scale_);
    model_->Update(camera);
}

void Player::BehaviorRootInitialize() {
    attackParameter_ = 0;
}

void Player::BehaviorRootUpdate() {
    HandleMoveInput();

    CollisionMapInfo info;
    info.velocity = velocity_;
    MapCollisionDetection(info);

    touchingTheCeiling(info);
    contactWithAWall(info);
    SwitchingInstallationStatus(info);

    // 攻撃への遷移 (Shiftキーなど)
    if (input_->Trigger(DIK_LSHIFT)) {
        behaviorRequest_ = Behavior::kAttack;
    }
}

void Player::BehaviorAttackInitialize() {
    attackPhase_ = AttackPhase::reservoir;
    attackParameter_ = 0;
    velocity_.x = 0;
}

void Player::BehaviorAttackUpdate() {
    // 簡易攻撃ロジック（溜め -> 突進 -> 残響）
    switch (attackPhase_) {
    case AttackPhase::reservoir:
        if (++attackParameter_ > 10) {
            attackPhase_ = AttackPhase::rush;
            attackParameter_ = 0;
            velocity_.x = (lrDirection_ == LRDirection::kRight) ? 0.3f : -0.3f;
        }
        break;
    case AttackPhase::rush:
        if (++attackParameter_ > 5) {
            attackPhase_ = AttackPhase::lingeringSound;
            attackParameter_ = 0;
        }
        break;
    case AttackPhase::lingeringSound:
        velocity_.x *= 0.8f;
        if (++attackParameter_ > 10) {
            behaviorRequest_ = Behavior::kRoot;
        }
        break;
    }

    // 攻撃中の移動と衝突判定
    CollisionMapInfo info;
    info.velocity = velocity_;
    MapCollisionDetection(info);
    SwitchingInstallationStatus(info);
}

void Player::HandleMoveInput() {
    if (onGround_) {
        // 地上移動
        if (input_->Press(DIK_D) || input_->Press(DIK_A)) {
            if (input_->Press(DIK_D)) {
                if (velocity_.x < 0.0f) velocity_.x *= (1.0f - kAttenuation);
                velocity_.x += kAcceleration;
                if (lrDirection_ != LRDirection::kRight) {
                    lrDirection_ = LRDirection::kRight;
                    turnTimer_ = kTimeTurn;
                }
            } else if (input_->Press(DIK_A)) {
                if (velocity_.x > 0.0f) velocity_.x *= (1.0f - kAttenuation);
                velocity_.x -= kAcceleration;
                if (lrDirection_ != LRDirection::kLeft) {
                    lrDirection_ = LRDirection::kLeft;
                    turnTimer_ = kTimeTurn;
                }
            }
            velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
        } else {
            velocity_.x *= (1.0f - kAttenuation);
        }

        // ジャンプ
        if (input_->Trigger(DIK_SPACE)) {
            velocity_.y = kJumpAcceleration;
            onGround_ = false;
        }
    } else {
        // 空中制御
        velocity_.y -= kGravityAccleration;
        velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);

        if (input_->Press(DIK_D)) velocity_.x = (std::min)(velocity_.x + 0.005f, kLimitRunSpeed);
        else if (input_->Press(DIK_A)) velocity_.x = (std::max)(velocity_.x - 0.005f, -kLimitRunSpeed);
    }
}

void Player::MapCollisionDetection(CollisionMapInfo& info) {
    if (!mapChipField_) return;

    // X軸判定
    CollisionMapInfo hInfo{};
    hInfo.velocity = { info.velocity.x, 0, 0 };
    MapCollisionDetectionLeft(hInfo);
    MapCollisionDetectionRight(hInfo);
    position_.x += hInfo.velocity.x;
    info.wallContactFlag = hInfo.wallContactFlag;

    // Y軸判定
    CollisionMapInfo vInfo{};
    vInfo.velocity = { 0, info.velocity.y, 0 };
    MapCollisionDetectionUp(vInfo);
    MapCollisionDetectionDown(vInfo);
    position_.y += vInfo.velocity.y;
    info.landingFlag = vInfo.landingFlag;
    info.ceilingCollisionFlag = vInfo.ceilingCollisionFlag;
    velocity_.y = vInfo.velocity.y; // 衝突後の速度を反映
}

// 各方向の衝突判定 (詳細)
void Player::MapCollisionDetectionUp(CollisionMapInfo& info) {
    if (info.velocity.y <= 0) return;
    std::vector<Corner> corners = { kLeftTop, kRightTop };
    for (auto corner : corners) {
        Vector3 pos = CornerPosition(position_ + info.velocity, corner);
        MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
            info.velocity.y = (std::max)(0.0f, rect.bottom - (position_.y + kHeight) - 0.001f);
            info.ceilingCollisionFlag = true;
            break;
        }
    }
}

void Player::MapCollisionDetectionDown(CollisionMapInfo& info) {
    if (info.velocity.y >= 0) return;
    std::vector<Corner> corners = { kLeftBottom, kRightBottom };
    for (auto corner : corners) {
        Vector3 pos = CornerPosition(position_ + info.velocity, corner);
        MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
            info.velocity.y = (std::min)(0.0f, rect.top - (position_.y - kHeight) + 0.001f);
            info.landingFlag = true;
            break;
        }
    }
}

void Player::MapCollisionDetectionLeft(CollisionMapInfo& info) {
    if (info.velocity.x >= 0) return;
    std::vector<Corner> corners = { kLeftBottom, kLeftTop };
    for (auto corner : corners) {
        Vector3 pos = CornerPosition(position_ + info.velocity, corner);
        MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
            info.velocity.x = (std::min)(0.0f, rect.right - (position_.x - kWidth) + 0.001f);
            info.wallContactFlag = true;
            break;
        }
    }
}

void Player::MapCollisionDetectionRight(CollisionMapInfo& info) {
    if (info.velocity.x <= 0) return;
    std::vector<Corner> corners = { kRightBottom, kRightTop };
    for (auto corner : corners) {
        Vector3 pos = CornerPosition(position_ + info.velocity, corner);
        MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
            info.velocity.x = (std::max)(0.0f, rect.left - (position_.x + kWidth) - 0.001f);
            info.wallContactFlag = true;
            break;
        }
    }
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
    const float xOffset = 0.0f; // 左右のズレがある場合はここを調整
    const float yOffset = 0.0f; // 上下のズレがある場合はここを調整

    switch (corner) {
    case kLeftBottom: return { (center.x + xOffset) - kWidth, (center.y + yOffset) - kHeight, center.z };
    case kRightBottom: return { (center.x + xOffset) + kWidth, (center.y + yOffset) - kHeight, center.z };
    case kLeftTop: return { (center.x + xOffset) - kWidth, (center.y + yOffset) + kHeight, center.z };
    case kRightTop: return { (center.x + xOffset) + kWidth, (center.y + yOffset) + kHeight, center.z };
    }
    return center;
}

void Player::touchingTheCeiling(const CollisionMapInfo& info) {
    if (info.ceilingCollisionFlag) velocity_.y = 0;
}

void Player::SwitchingInstallationStatus(const CollisionMapInfo& info) {
    if (onGround_) {
        if (velocity_.y > 0.0f) {
            onGround_ = false;
        } else {
            // 足元が崖になっていないか確認
            CollisionMapInfo checkInfo{};
            checkInfo.velocity = { 0, -0.01f, 0 };
            MapCollisionDetectionDown(checkInfo);
            if (!checkInfo.landingFlag) onGround_ = false;
        }
    } else {
        if (info.landingFlag) {
            onGround_ = true;
            velocity_.x *= (1.0f - kAttenuationLanding);
            velocity_.y = 0.0f;
        }
    }
}

void Player::contactWithAWall(const CollisionMapInfo& info) {
    if (info.wallContactFlag && !onGround_) {
        velocity_.x = 0; // 壁にぶつかったら止まる
    }
}

void Player::Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight) {
    model_->Draw(
        dxCommon->GetCommandList(),
        textureManager->GetGpuHandle("player"),
        dirLight->GetGPUVirtualAddress(),
        psoManager->GetPSO("Object3D"),
        psoManager->GetRootSignature("Object3D"),
        true
    );
}
