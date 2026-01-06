#pragma once
#include "Struct.h"
#include "Camera.h"

class Player;

class CameraControl {
public:
    void Initialize(Camera* camera, Player* player);
    void Update();

private:
    Camera* camera_ = nullptr;
    Player* player_ = nullptr;

    // プレイヤーからのオフセット距離（お好みの値に調整してください）
    Vector3 offset_ = { 0.0f, 1.0f, -5.0f };
    // 追従の滑らかさ (0.0f ～ 1.0f)
    float lerpFactor_ = 0.1f;
};
