#include "Enemy.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void Enemy::Initialize(DxCommon* dxCommon, TextureManager* textureManager, const Vector3& position) {
    position_ = position;
    // モデルの生成 (パスは環境に合わせてください)
    model_ = std::make_unique<ModelObject>(dxCommon->GetDevice(), "resources/AL/enemy/enemy.obj", position_);

    // テクスチャ読み込み
    textureManager->LoadTexture("enemy", "resources/AL/enemy/enemy.png");

    velocity_ = { -0.01f, 0.0f, 0.0f };
    behavior_ = Behavior::kRoot;
}

void Enemy::Update(Camera* camera) {
    float param = 0.0f;
    switch (behavior_) {
    case Behavior::kRoot:
        walkTimer_ += 1.0f / 60.0f;
        // 左右に揺れるアニメーション
        param = std::sin(M_PI * walkTimer_ / kWalkMotionTime);
        rotation_.z = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;

        position_.x += velocity_.x;
        position_.y += velocity_.y;
        break;

    case Behavior::kDead:
        isCollisionDisabled_ = true;
        deadTimer_ += 1.0f / 60.0f;
        float t = (std::min)(deadTimer_ / 0.5f, 1.0f); // 0.5秒で消える演出

        // 潰れるような演出
        scale_.x = 0.1f + (t * 0.1f);
        scale_.y = 0.1f - (t * 0.08f);

        if (t >= 1.0f) isDead_ = true;
        break;
    }

    model_->SetPosition(position_);
    model_->SetRotate(rotation_);
    model_->SetScale(scale_);
    model_->Update(camera);
}

void Enemy::Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight) {
    if (isDead_) return;
    model_->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle("enemy"),
        dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"),
        psoManager->GetRootSignature("Object3D"), true);
}

Enemy::AABB Enemy::GetAABB() const {
    return {
        { position_.x - kWidth, position_.y - kHeight, position_.z - kWidth },
        { position_.x + kWidth, position_.y + kHeight, position_.z + kWidth }
    };
}

void Enemy::OnCollisionWithPlayer() {
    if (behavior_ != Behavior::kDead) {
        behavior_ = Behavior::kDead;
    }
}
