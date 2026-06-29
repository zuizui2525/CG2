#include "App/Scene/Game/Player.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Base/BaseResource.h"

Player::Player() {
    cube_ = std::make_unique<CubeObject>();
    weaponCube_ = std::make_unique<CubeObject>();
    bulletEffectName_ = kBulletEffectName;
}

void Player::Initialize() {
    cube_->Initialize();
    cube_->SetPosition(kInitialPosition);
    cube_->SetSize(kPlayerScale);
    cube_->SetColor(kPlayerColor); // プレイヤーを青くする

    weaponCube_->Initialize();
    weaponCube_->SetSize(kWeaponScale);
    weaponCube_->SetColor(kWeaponColor);

    ClearBullets();
    InitializeHpBar(); // 体力と体力バーの初期化
    isAutoMoving_ = false; // 自動走行フラグをリセット
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

    // 自動走行中でない場合のみ手動での左右移動を有効化する
    if (!isAutoMoving_) {
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
    }
    
    cube_->Update();

    // 弾の発射処理 (SPACEキー)
    if (input->Trigger(kShotKey)) {
        // 武器先端（銃口）から発射する
        static const float kMuzzleOffsetZ = 0.8f;
        Vector3 muzzlePos = Math::Add(weaponPosition_, Math::Multiply(kMuzzleOffsetZ, direction_));

        // 画面中央（カメラの視線線上かつ十分前方）に弾を誘導する
        static const float kTargetDistance = 50.0f;
        Vector3 targetPos = Math::Add(cameraPosition_, Math::Multiply(kTargetDistance, direction_));

        // 銃口から画面中央の照準先ターゲットへの方向ベクトル
        Vector3 bulletDir = Math::Normalize(Math::Subtract(targetPos, muzzlePos));
        Vector3 bulletVel = Math::Multiply(kBulletSpeed, bulletDir);

        bullets_.push_back(std::make_unique<Bullet>(muzzlePos, bulletVel, kBulletEffectName));
    }

    // 弾の更新
    UpdateBullets();

    // 体力バーの追従更新
    UpdateHpBar(pos);
}

void Player::Draw() {
    // プレイヤー本体の描画
    if (cube_) {
        cube_->Draw(kTextureKey, kEnvMapKey);
    }
    
    // 武器モデルの描画
    if (weaponCube_) {
        weaponCube_->Draw(kTextureKey, kEnvMapKey);
    }
    
    DrawBullets();
    DrawHpBar(); // 体力バーの描画
}

void Player::UpdateWeapon(const Matrix4x4& viewMatrix) {
    if (!weaponCube_) {
        return;
    }

    // カメラのビュー行列の逆行列（カメラのワールド行列）を求める
    Matrix4x4 camWorld = Math::Inverse(viewMatrix);

    // カメラの右・上・前ベクトルとカメラ位置を抽出
    Vector3 right   = { camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] };
    Vector3 up      = { camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2] };
    Vector3 forward = { camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2] };
    Vector3 camPos  = { camWorld.m[3][0], camWorld.m[3][1], camWorld.m[3][2] };

    // メンバ変数に保存
    cameraPosition_ = camPos;
    direction_ = Math::Normalize(forward); // 向きベクトルを正規化

    // 武器のワールド座標を計算 (カメラ位置からローカルオフセット分移動)
    Vector3 rightOffset = Math::Multiply(kWeaponLocalOffset.x, right);
    Vector3 upOffset    = Math::Multiply(kWeaponLocalOffset.y, up);
    Vector3 forwardOffset = Math::Multiply(kWeaponLocalOffset.z, forward);
    
    Vector3 weaponPos = Math::Add(camPos, Math::Add(rightOffset, Math::Add(upOffset, forwardOffset)));
    weaponPosition_ = weaponPos;

    // 前方向ベクトルからピッチとヨーを逆算して、武器の回転角度を計算する
    float yaw = std::atan2(forward.x, forward.z);
    float pitch = -std::asin(forward.y);

    // 武器の回転を設定 (銃口を画面中央に寄せるヨーオフセットを適用)
    Vector3 weaponRot = { pitch, yaw + kWeaponYawOffset, 0.0f };

    weaponCube_->SetPosition(weaponPos);
    weaponCube_->SetRotate(weaponRot);
    weaponCube_->Update();
}
