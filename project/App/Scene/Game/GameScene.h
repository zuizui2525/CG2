#pragma once
#include "App/Scene/Core/IScene.h"
#include <memory>
#include <string>

class PostProcess;

// エンジンコンポーネントのインクルード
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"
#include "App/Scene/Game/Player.h"
#include "App/Scene/Game/Enemy.h"

/**
 * @brief ゲーム本編を管理するシーンクラス
 */
class GameScene : public IScene {
public:
    // コンストラクタ・デストラクタ
    GameScene() = default;
    ~GameScene() override = default;

    // IScene インターフェースの仮想関数オーバーライド
    void Initialize() override;     // 初期化処理
    void ImGuiControl() override;   // デバッグ用ImGui描画処理
    void Update() override;         // 毎フレーム更新処理
    void Draw() override;           // 毎フレーム描画処理

private:
    // マジックナンバーを排除するための定数宣言
    static inline const std::string kMainCameraName = "Main";       // メインカメラの登録・選択用キー
    static inline const std::string kDebugCameraName = "Debug";     // デバッグカメラの登録・選択用キー
    static inline const std::string kClearSceneName = "Clear";       // クリアシーンへの遷移用キー
    static inline const std::string kGameOverSceneName = "GameOver"; // ゲームオーバーシーンへの遷移用キー

private:
    // エンジンの各種マネージャへの生ポインタ（所有権は持たない）
    Input* input_ = nullptr;            // キー入力マネージャ
    CameraManager* cameraMgr_ = nullptr; // カメラ管理者
    LightManager* lightMgr_ = nullptr;  // ライト管理者
    PostProcess* postProcess_ = nullptr;

    // シーン内で作成・管理するオブジェクト
    std::shared_ptr<BaseCamera> mainCamera_;        // メインカメラ
    std::shared_ptr<DebugCamera> debugCamera_;      // デバッグ確認用フリーカメラ
    std::unique_ptr<DirectionalLightObject> dirLight_; // 平行光源
    std::unique_ptr<Player> player_;                // プレイヤーオブジェクト
    std::unique_ptr<Enemy> enemy_;                  // 敵オブジェクト

    // AABB衝突判定関数
    bool IsCollidingAABB(const Vector3& pos1, const Vector3& size1, const Vector3& pos2, const Vector3& size2) const;

    // シェイク機能用変数と定数
    int shakeTimer_ = 0;
    static inline const Vector3 kDefaultCameraPos = { 0.0f, 4.0f, -20.0f }; // メインカメラの基準位置
    static inline const int kShakeDuration = 15;                             // シェイクフレーム数
    static inline const float kShakeIntensity = 0.2f;                        // シェイクの強さ
    static inline const std::string kPlayerBulletHitEffectName = "YellowFire"; // プレイヤー弾ヒット時のエフェクト名（黄色炎）
    static inline const std::string kEnemyBulletHitEffectName = "PurpleFire"; // 敵弾ヒット時のエフェクト名（紫色炎）
};

