#pragma once
#include <vector>
#include <memory>
#include <cstdlib>
#include "Engine/Math/MathStructs.h"
#include "App/Scene/Game/Bullet.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"

/**
 * @brief 弾を発射・管理するオブジェクトの共通基底クラス
 */
class Shooter {
public:
    virtual ~Shooter() = default;

    virtual void Initialize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    /**
     * @brief 弾の更新処理
     */
    void UpdateBullets() {
        for (auto it = bullets_.begin(); it != bullets_.end();) {
            (*it)->Update();
            if (!(*it)->IsActive()) {
                it = bullets_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief 弾の描画処理
     */
    void DrawBullets() {
        for (auto& bullet : bullets_) {
            bullet->Draw();
        }
    }

    /**
     * @brief 弾の全クリア
     */
    void ClearBullets() {
        bullets_.clear();
    }

    /**
     * @brief 弾のリストを取得
     */
    const std::vector<std::unique_ptr<Bullet>>& GetBullets() const {
        return bullets_;
    }

    // 衝突判定および位置調整用の仮想関数
    virtual Vector3 GetPosition() const = 0;
    virtual Vector3 GetSize() const = 0;

    // 体力管理API
    int GetHp() const { return hp_; }
    void Damage(int amount, const std::string& effectName) {
        hp_ -= amount;
        if (hp_ < 0) {
            hp_ = 0;
        }
        burnTimer_ = kBurnDuration; // 燃焼タイマーを設定
        burnEffectName_ = effectName; // 燃焼エフェクト名を設定
    }
    bool IsDead() const { return hp_ <= 0; }

protected:
    /**
     * @brief 体力バーの初期化
     */
    void InitializeHpBar() {
        hp_ = kMaxHp;
        burnTimer_ = 0;
        
        hpBarBG_ = std::make_unique<CubeObject>();
        hpBarBG_->Initialize();
        hpBarBG_->SetSize(kHpBarScale);
        hpBarBG_->SetColor(kHpBarBgColor);

        hpBarFill_ = std::make_unique<CubeObject>();
        hpBarFill_->Initialize();
        hpBarFill_->SetSize(kHpBarScale);
        hpBarFill_->SetColor(kHpBarFillColor);
    }

    /**
     * @brief 体力バーの追従更新
     */
    void UpdateHpBar(const Vector3& charPos) {
        if (!hpBarBG_ || !hpBarFill_) return;

        // 背景バーの位置（キャラクターの頭の上）
        Vector3 bgPos = charPos + kHpBarOffset;
        hpBarBG_->SetPosition(bgPos);
        hpBarBG_->Update();

        // 前景バーのスケールと位置の調整
        float hpRate = static_cast<float>(hp_) / static_cast<float>(kMaxHp);
        
        Vector3 fillScale = kHpBarScale;
        fillScale.x *= hpRate;
        hpBarFill_->SetSize(fillScale);

        // 左揃えにするためのオフセット計算
        static constexpr float kHalf = 0.5f;
        Vector3 fillPos = bgPos;
        fillPos.x -= (kHpBarScale.x - fillScale.x) * kHalf;
        
        // 重なりによるチラつき（Z-Fighting）を防ぐため、少し手前にずらす
        static constexpr float kZOffset = -0.01f; 
        fillPos.z += kZOffset;

        hpBarFill_->SetPosition(fillPos);
        hpBarFill_->Update();

        // 燃焼エフェクト（全身炎）の更新処理
        if (burnTimer_ > 0) {
            burnTimer_--;
            for (int i = 0; i < kFireCountPerFrame; ++i) {
                Vector3 size = GetSize();
                float rx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * size.x;
                float ry = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * size.y;
                float rz = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * size.z;

                Vector3 firePos = charPos + Vector3{ rx, ry, rz };
                EffectPlayParam burnParam;
                burnParam.position = firePos;
                burnParam.scale = kBurnEffectScale;
                EffectManager::GetInstance()->PlayEffect2D(burnEffectName_, burnParam);
            }
        }
    }

    /**
     * @brief 体力バーの描画
     */
    void DrawHpBar() {
        if (hpBarBG_) {
            hpBarBG_->Draw(kTextureKey, kEnvMapKey);
        }
        if (hpBarFill_ && hp_ > 0) {
            hpBarFill_->Draw(kTextureKey, kEnvMapKey);
        }
    }

protected:
    std::vector<std::unique_ptr<Bullet>> bullets_;
    int hp_ = kMaxHp;
    int burnTimer_ = 0;
    std::string bulletEffectName_ = "Fire";
    std::string burnEffectName_ = "Fire";

    std::unique_ptr<CubeObject> hpBarBG_;
    std::unique_ptr<CubeObject> hpBarFill_;

    // マジックナンバー排除のための定数
    static inline const int kMaxHp = 10;                                      // 最大体力
    static inline const int kBurnDuration = 30;                               // 燃焼フレーム数
    static inline const int kFireCountPerFrame = 3;                           // 1フレームあたりの発生炎数
    static inline const Vector3 kBurnEffectScale = { 1.5f, 1.5f, 1.5f };      // 炎エフェクトのスケール
    static inline const std::string kFireEffectName = "Fire";                 // 炎エフェクト名
    static inline const Vector3 kHpBarOffset = { 0.0f, 1.3f, 0.0f };         // キャラクター頭上へのオフセット
    static inline const Vector3 kHpBarScale = { 1.2f, 0.1f, 0.1f };           // バーの基本サイズ
    static inline const Vector4 kHpBarBgColor = { 0.3f, 0.0f, 0.0f, 1.0f };   // 背景（暗い赤）
    static inline const Vector4 kHpBarFillColor = { 0.0f, 0.8f, 0.0f, 1.0f }; // 前景（緑）
    static inline const std::string kTextureKey = "white";
    static inline const std::string kEnvMapKey = "";
};
