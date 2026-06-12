#include "App/Scene/Game/Player.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Base/BaseResource.h"

Player::Player() {
    cube_ = std::make_unique<CubeObject>();
}

void Player::Initialize() {
    cube_->Initialize();
    cube_->SetPosition(kInitialPosition);
    cube_->SetSize(kPlayerScale);
    ClearBullets();
}

void Player::Update() {
    Input* input = InputResource::GetInput();
    UpdateInput(input);
}

void Player::UpdateInput(Input* input) {
    if (!input) {
        return;
    }

    Vector3 pos = cube_->GetPosition();

    // 左右移動処理 (ADキー)
    if (input->Press(kMoveLeftKey)) {
        pos.x -= kSpeed;
    }
    if (input->Press(kMoveRightKey)) {
        pos.x += kSpeed;
    }

    // 移動制限
    if (pos.x < -kMoveLimitX) {
        pos.x = -kMoveLimitX;
    }
    if (pos.x > kMoveLimitX) {
        pos.x = kMoveLimitX;
    }

    cube_->SetPosition(pos);
    cube_->Update();

    // 弾の発射処理 (SPACEキー)
    if (input->Trigger(kShotKey)) {
        // プレイヤーの少し前方から弾を発射する
        Vector3 bulletPos = pos + Vector3{ 0.0f, 0.0f, 1.0f };
        bullets_.push_back(std::make_unique<Bullet>(bulletPos, kBulletVelocity));
    }

    // 弾の更新
    UpdateBullets();
}

void Player::Draw() {
    cube_->Draw(kTextureKey, kEnvMapKey);
    DrawBullets();
}
