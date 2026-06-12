#include "App/Scene/Game/Enemy.h"
#include "Engine/Math/Matrix/Matrix.h"

Enemy::Enemy() {
    cube_ = std::make_unique<CubeObject>();
    bulletEffectName_ = kBulletEffectName;
    
    // 乱数シードの設定
    std::random_device seed_gen;
    randomEngine_ = std::mt19937(seed_gen());
}

void Enemy::Initialize() {
    cube_->Initialize();
    cube_->SetPosition(kInitialPosition);
    cube_->SetSize(kEnemyScale);
    cube_->SetColor(kEnemyColor); // 敵を赤くする
    ClearBullets();
    InitializeHpBar(); // 体力と体力バーの初期化

    // タイマーおよび方向の初期設定
    std::uniform_int_distribution<int> distMove(kMinMoveTime, kMaxMoveTime);
    moveTimer_ = distMove(randomEngine_);

    std::uniform_int_distribution<int> distShot(kMinShotTime, kMaxShotTime);
    shotTimer_ = distShot(randomEngine_);

    std::uniform_int_distribution<int> distDir(0, 1);
    moveDirection_ = (distDir(randomEngine_) == 0) ? -1.0f : 1.0f;
}

void Enemy::Update() {
    Vector3 pos = cube_->GetPosition();

    // 移動方向転換タイマーの更新
    moveTimer_--;
    if (moveTimer_ <= 0) {
        std::uniform_int_distribution<int> distMove(kMinMoveTime, kMaxMoveTime);
        moveTimer_ = distMove(randomEngine_);

        std::uniform_int_distribution<int> distDir(0, 1);
        moveDirection_ = (distDir(randomEngine_) == 0) ? -1.0f : 1.0f;
    }

    // 左右移動
    pos.x += moveDirection_ * kSpeed;

    // 移動制限と衝突による方向反転
    if (pos.x < -kMoveLimitX) {
        pos.x = -kMoveLimitX;
        moveDirection_ = 1.0f;
    }
    if (pos.x > kMoveLimitX) {
        pos.x = kMoveLimitX;
        moveDirection_ = -1.0f;
    }

    cube_->SetPosition(pos);
    cube_->Update();

    // 弾の発射タイマーの更新
    shotTimer_--;
    if (shotTimer_ <= 0) {
        std::uniform_int_distribution<int> distShot(kMinShotTime, kMaxShotTime);
        shotTimer_ = distShot(randomEngine_);

        // 敵の少し手前から弾を発射する
        Vector3 bulletPos = pos + Vector3{ 0.0f, 0.0f, -1.0f };
        bullets_.push_back(std::make_unique<Bullet>(bulletPos, kBulletVelocity, kBulletEffectName));
    }

    // 弾の更新
    UpdateBullets();

    // 体力バーの追従更新
    UpdateHpBar(pos);
}

void Enemy::Draw() {
    cube_->Draw(kTextureKey, kEnvMapKey);
    DrawBullets();
    DrawHpBar(); // 体力バーの描画
}
