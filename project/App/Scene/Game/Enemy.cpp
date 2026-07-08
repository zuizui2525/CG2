#include "App/Scene/Game/Enemy.h"
#include "App/Scene/Game/Player.h"
#include "Engine/Math/Matrix/Matrix.h"

Enemy::Enemy() {
    cube_ = std::make_unique<CubeObject>();
    headCube_ = std::make_unique<CubeObject>();
    bodyCollider_ = std::make_unique<PartCollider>();
    headCollider_ = std::make_unique<PartCollider>();
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

    // 頭部モデルの初期化と親子関係設定
    headCube_->Initialize();
    headCube_->SetParent(cube_.get());
    headCube_->SetPosition(kHeadLocalPos);
    headCube_->SetScale(kHeadLocalScale);
    headCube_->SetColor(kHeadColor);

    // 部位コライダーの初期化と親子関係設定
    bodyCollider_->Initialize(PartCollider::Type::Body, kBodyColliderSize);
    bodyCollider_->SetParent(cube_.get());
    bodyCollider_->SetPosition({ 0.0f, 0.0f, 0.0f });
    bodyCollider_->SetScale({ 1.0f, 1.0f, 1.0f });

    headCollider_->Initialize(PartCollider::Type::Head, kHeadColliderSize);
    headCollider_->SetParent(cube_.get());
    headCollider_->SetPosition(kHeadLocalPos);
    headCollider_->SetScale({ 1.0f, 1.0f, 1.0f });

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

    // 子オブジェクトおよびコライダーの更新 (親子関係の行列更新を反映)
    headCube_->Update();
    bodyCollider_->Update();
    headCollider_->Update();

    // 弾の発射タイマーの更新
    shotTimer_--;
    if (shotTimer_ <= 0) {
        std::uniform_int_distribution<int> distShot(kMinShotTime, kMaxShotTime);
        shotTimer_ = distShot(randomEngine_);

        // 敵の少し手前から弾を発射する
        Vector3 bulletPos = pos + Vector3{ 0.0f, 0.0f, -1.0f };
        Vector3 bulletVel = kBulletVelocity;

        // 自機（プレイヤー）が登録されている場合は、自機を狙って弾を発射
        if (targetPlayer_) {
            Vector3 toPlayer = Math::Subtract(targetPlayer_->GetPosition(), bulletPos);
            static constexpr float kBulletSpeedVal = 0.15f;
            bulletVel = Math::Multiply(kBulletSpeedVal, Math::Normalize(toPlayer));
        }

        bullets_.push_back(std::make_unique<Bullet>(bulletPos, bulletVel, kBulletEffectName));
    }

    // 弾の更新
    UpdateBullets();

    // 体力バーの追従更新 (頭部の上に表示するため、少し上にオフセット)
    static const Vector3 kHpOffset = { 0.0f, 1.8f, 0.0f };
    UpdateHpBar(pos + kHpOffset);
}

void Enemy::Draw() {
    cube_->Draw(kTextureKey, kEnvMapKey);
    headCube_->Draw(kTextureKey, kEnvMapKey); // 頭部モデルの描画
    DrawBullets();
    DrawHpBar(); // 体力バーの描画
}
