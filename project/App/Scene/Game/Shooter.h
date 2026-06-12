#pragma once
#include <vector>
#include <memory>
#include "Engine/Math/MathStructs.h"
#include "App/Scene/Game/Bullet.h"

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

protected:
    std::vector<std::unique_ptr<Bullet>> bullets_;
};
