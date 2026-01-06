#pragma once
#include "ModelObject.h"
#include "Camera.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include <memory>

// 前方宣言
class Player;

class Enemy {
private:
    enum class Behavior {
        kRoot,    // 通常状態
        kDead,    // 死亡演出中
    };

    // メンバ変数（あなたのプロジェクトの形式に合わせる）
    std::unique_ptr<ModelObject> model_;
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 scale_ = { 0.1f, 0.1f, 0.1f }; // Playerに合わせて0.1程度に
    Vector3 velocity_ = { -0.01f, 0.0f, 0.0f }; // 速度調整

    // アニメーション用
    static inline const float kWalkMotionAngleStart = -0.2f; // ラジアン
    static inline const float kWalkMotionAngleEnd = 0.4f;
    static inline const float kWalkMotionTime = 0.5f;
    float walkTimer_ = 0.0f;

    // 当たり判定サイズ
    static inline const float kWidth = 0.2f;
    static inline const float kHeight = 0.2f;

    bool isDead_ = false; // 完全に消滅するフラグ
    bool isCollisionDisabled_ = false;
    Behavior behavior_ = Behavior::kRoot;
    float deadTimer_ = 0.0f;

public:
    void Initialize(DxCommon* dxCommon, TextureManager* textureManager, const Vector3& position);
    void Update(Camera* camera);
    void Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight);

    // ゲッター
    Vector3 GetPosition() const { return position_; }
    bool IsDead() const { return isDead_; }

    // 当たり判定用
    struct AABB {
        Vector3 min;
        Vector3 max;
    };
    AABB GetAABB() const;
    void OnCollisionWithPlayer();
};
