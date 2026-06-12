#include "App/Scene/Game/Bullet.h"
#include "Engine/Math/Matrix/Matrix.h"

Bullet::Bullet(const Vector3& position, const Vector3& velocity)
    : velocity_(velocity) {
    cube_ = std::make_unique<CubeObject>();
    cube_->Initialize();
    cube_->SetPosition(position);
    cube_->SetSize(kBulletScale);
}

void Bullet::Initialize() {
    // コンストラクタで初期化しているため空処理
}

void Bullet::Update() {
    if (!isActive_) {
        return;
    }

    // 移動処理
    cube_->SetPosition(cube_->GetPosition() + velocity_);
    cube_->Update();

    // 寿命タイマーによる消滅判定
    lifeTimer_++;
    if (lifeTimer_ >= kMaxLifeTime) {
        isActive_ = false;
    }

    // Z軸の画面外境界による消滅判定
    float z = cube_->GetPosition().z;
    if (z > kZBoundary || z < -kZBoundary) {
        isActive_ = false;
    }
}

void Bullet::Draw() {
    if (!isActive_) {
        return;
    }
    // 描画
    cube_->Draw(kTextureKey, kEnvMapKey);
}
