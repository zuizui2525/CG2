#include "CameraControl.h"
#include "Player.h"

void CameraControl::Initialize(Camera* camera, Player* player) {
    camera_ = camera;
    player_ = player;
}

void CameraControl::Update() {
    if (!camera_ || !player_) return;

    // 1. プレイヤーの現在座標を取得
    Vector3 playerPos = player_->GetPosition();

    // 2. 目標座標を計算
    Vector3 targetPos;
    targetPos.x = playerPos.x + offset_.x;
    targetPos.y = playerPos.y + offset_.y;
    targetPos.z = playerPos.z + offset_.z;

    // 3. 現在のカメラ座標を取得
    auto transform = camera_->GetTransform();
    Vector3 currentPos = transform.translate;

    // 4. 線形補間(Lerp)で滑らかに近づける
    transform.translate.x = currentPos.x + (targetPos.x - currentPos.x) * lerpFactor_;
    transform.translate.y = currentPos.y + (targetPos.y - currentPos.y) * lerpFactor_;
    transform.translate.z = currentPos.z + (targetPos.z - currentPos.z) * lerpFactor_;

    // 5. カメラに反映
    camera_->SetTransform(transform);
}
