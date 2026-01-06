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
    model_ = std::make_unique<ModelObject>(dxCommon->GetDevice(), "resources/AL/player/player.obj", position_);
    rotation_.y = M_PI / 2.0f;
    textureManager->LoadTexture("player", model_->GetModelData()->material.textureFilePath);
}

void Player::Update(Camera* camera) {
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;
        if (behavior_ == Behavior::kRoot) BehaviorRootInitialize();
        else if (behavior_ == Behavior::kAttack) BehaviorAttackInitialize();
        behaviorRequest_ = Behavior::kUnknown;
    }

    if (behavior_ == Behavior::kAttack) {
        BehaviorAttackUpdate();
    } else {
        BehaviorRootUpdate();
    }

    // --- 顔の向き（旋回）の更新 ---
    if (turnTimer_ > 0.0f) {
        turnTimer_ -= 1.0f / 60.0f;

        // 方向に応じて目標の角度を決定
        // もし逆なら、M_PI/2.0f と M_PI*3.0f/2.0f を入れ替える
        float destRotY = (lrDirection_ == LRDirection::kRight) ? (M_PI * 3.0f / 2.0f) : (M_PI / 2.0f);

        // 角度を滑らかに補間
        rotation_.y += (destRotY - rotation_.y) * 0.2f;
    }

    model_->SetPosition(position_);
    model_->SetRotate(rotation_);
    model_->SetScale(scale_);
    model_->Update(camera);
}

void Player::BehaviorRootInitialize() { attackParameter_ = 0; }

void Player::BehaviorRootUpdate() {
    HandleMoveInput();
    CollisionMapInfo info;
    info.velocity = velocity_;
    MapCollisionDetection(info);
    touchingTheCeiling(info);
    contactWithAWall(info);
    SwitchingInstallationStatus(info);

    if (input_->Trigger(DIK_LSHIFT)) behaviorRequest_ = Behavior::kAttack;
}

void Player::BehaviorAttackInitialize() {
    attackPhase_ = AttackPhase::reservoir;
    attackParameter_ = 0;
    velocity_.x = 0;
}

void Player::BehaviorAttackUpdate() {
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
        if (++attackParameter_ > 10) behaviorRequest_ = Behavior::kRoot;
        break;
    }
    CollisionMapInfo info;
    info.velocity = velocity_;
    MapCollisionDetection(info);
    SwitchingInstallationStatus(info);
}

void Player::HandleMoveInput() {
    if (wallJumpTimer_ > 0.0f) {
        wallJumpTimer_ -= 1.0f / 60.0f;
    }

    if (onGround_) {
        // --- 地上処理 ---
        canDoubleJump_ = true;
        isWallJumping_ = false;

        // 左右移動（地上）
        if (input_->Press(DIK_D) || input_->Press(DIK_A)) {
            if (input_->Press(DIK_D)) {
                velocity_.x = (std::min)(velocity_.x + kAcceleration, kLimitRunSpeed);
                lrDirection_ = LRDirection::kRight;
                turnTimer_ = kTimeTurn;
            } else {
                velocity_.x = (std::max)(velocity_.x - kAcceleration, -kLimitRunSpeed);
                lrDirection_ = LRDirection::kLeft;
                turnTimer_ = kTimeTurn;
            }
        } else {
            velocity_.x *= (1.0f - kAttenuation);
        }

        // 地上ジャンプ
        if (input_->Trigger(DIK_SPACE)) {
            velocity_.y = kJumpAcceleration;
            onGround_ = false;
        }
    } else {
        // --- 空中処理 ---
        if (wallJumpTimer_ <= 0.0f) {
            if (input_->Press(DIK_D)) {
                // 右移動
                velocity_.x = (std::min)(velocity_.x + 0.005f, kLimitRunSpeed);
                // 【追加】向きを右に更新し、旋回タイマーをセット
                lrDirection_ = LRDirection::kRight;
                turnTimer_ = kTimeTurn;
            } else if (input_->Press(DIK_A)) {
                // 左移動
                velocity_.x = (std::max)(velocity_.x - 0.005f, -kLimitRunSpeed);
                // 【追加】向きを左に更新し、旋回タイマーをセット
                lrDirection_ = LRDirection::kLeft;
                turnTimer_ = kTimeTurn;
            }
        }

        // 重力適用
        velocity_.y = (std::max)(velocity_.y - kGravityAccleration, -kLimitFallSpeed);

        // 空中でのジャンプ入力
        if (input_->Trigger(DIK_SPACE)) {
            // 1. まず壁ジャンプができるか確認
            CollisionMapInfo wallCheck;
            // 左右に少しだけ判定を出して壁があるか見る
            float checkDir = (lrDirection_ == LRDirection::kRight) ? 0.02f : -0.02f;
            wallCheck.velocity = { checkDir, 0, 0 };

            // 注意：ここで position_ を更新せず、判定だけ行う
            MapCollisionDetectionLeft(wallCheck);
            MapCollisionDetectionRight(wallCheck);

            if (wallCheck.wallContactFlag) {
                // 壁ジャンプ成功
                velocity_.y = kJumpAcceleration;
                velocity_.x = (lrDirection_ == LRDirection::kRight) ? -kLimitRunSpeed : kLimitRunSpeed;
                lrDirection_ = (lrDirection_ == LRDirection::kRight) ? LRDirection::kLeft : LRDirection::kRight;
                turnTimer_ = kTimeTurn;
                wallJumpTimer_ = 0.1f;
                isWallJumping_ = true;
                // 壁ジャンプしたら2段ジャンプ権も復活させてあげるとプレイ感が良くなります（お好みで）
                canDoubleJump_ = true;
            }
            // 2. 壁ジャンプ不可なら2段ジャンプを試みる
            else if (canDoubleJump_) {
                velocity_.y = kJumpAcceleration;
                canDoubleJump_ = false;
                isWallJumping_ = false;
            }
            // 3. どちらも不可なら何もしない（これで「カクッ」が消えます）
        }
    }
}

void Player::MapCollisionDetection(CollisionMapInfo& info) {
    if (!mapChipField_) return;
    CollisionMapInfo hInfo{};
    hInfo.velocity = { info.velocity.x, 0, 0 };
    MapCollisionDetectionLeft(hInfo);
    MapCollisionDetectionRight(hInfo);
    position_.x += hInfo.velocity.x;
    info.wallContactFlag = hInfo.wallContactFlag;

    CollisionMapInfo vInfo{};
    vInfo.velocity = { 0, info.velocity.y, 0 };
    MapCollisionDetectionUp(vInfo);
    MapCollisionDetectionDown(vInfo);
    position_.y += vInfo.velocity.y;
    info.landingFlag = vInfo.landingFlag;
    info.ceilingCollisionFlag = vInfo.ceilingCollisionFlag;
    velocity_.y = vInfo.velocity.y;
}

void Player::MapCollisionDetectionUp(CollisionMapInfo& info) {
    if (info.velocity.y <= 0) return;
    std::vector<Corner> corners = { kLeftTop, kRightTop };
    for (auto corner : corners) {
        Vector3 pos = CornerPosition(position_ + info.velocity, corner);
        auto index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            auto rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
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
        auto index = mapChipField_->GetMapChipIndexSetByPosition(pos);

        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            auto rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

            float destinationY = rect.top + kHeight + 0.001f;
            info.velocity.y = destinationY - position_.y;
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
        auto index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            auto rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
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
        auto index = mapChipField_->GetMapChipIndexSetByPosition(pos);
        if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipField::MapChipType::kBlock) {
            auto rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);
            info.velocity.x = (std::max)(0.0f, rect.left - (position_.x + kWidth) - 0.001f);
            info.wallContactFlag = true;
            break;
        }
    }
}

void Player::touchingTheCeiling(const CollisionMapInfo& info) { if (info.ceilingCollisionFlag) velocity_.y = 0; }

void Player::SwitchingInstallationStatus(const CollisionMapInfo& info) {
    if (onGround_) {
        if (velocity_.y > 0.0f) {
            onGround_ = false;
        } else {
            CollisionMapInfo checkInfo;
            checkInfo.velocity = { 0, -0.02f, 0 };
            MapCollisionDetectionDown(checkInfo);
            if (!checkInfo.landingFlag) {
                onGround_ = false;
            }
        }
    } else {
        if (info.landingFlag) {
            onGround_ = true;
            velocity_.x *= (1.0f - kAttenuationLanding);
            velocity_.y = 0.0f;
        }
    }
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
    switch (corner) {
    case kLeftBottom:  return { center.x - kWidth, center.y - kHeight, center.z };
    case kRightBottom: return { center.x + kWidth, center.y - kHeight, center.z };
    case kLeftTop:     return { center.x - kWidth, center.y + kHeight, center.z };
    case kRightTop:    return { center.x + kWidth, center.y + kHeight, center.z };
    }
    return center;
}

void Player::contactWithAWall(const CollisionMapInfo& info) {
    if (info.wallContactFlag && !onGround_) {
        velocity_.x = 0;
    }
}

void Player::Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight) {
    model_->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle("player"), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}
