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
#include "Engine/Graphics/Objects/3d/Line/LineObject.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"

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
    // ゲームモード定義
    enum class GameMode {
        DrawRoute, // ルート描画モード
        Play       // 走行プレイモード
    };

    // マジックナンバーを排除するための定数宣言
    static inline const std::string kMainCameraName = "Main";       // メインカメラの登録・選択用キー
    static inline const std::string kDebugCameraName = "Debug";     // デバッグカメラの登録・選択用キー
    static inline const std::string kClearSceneName = "Clear";       // クリアシーンへの遷移用キー
    static inline const std::string kGameOverSceneName = "GameOver"; // ゲームオーバーシーンへの遷移用キー
    
    // ルート描画・走行用の定数
    static inline const float kMinPointDistance = 2.0f;              // 軌跡点間の最小距離
    static inline const Vector3 kTopDownCameraPos = { 0.0f, 120.0f, 0.0f }; // 上空見下ろしカメラ位置（スタート・ゴールが画面内に確実に収まる高さ）
    static inline const Vector3 kTopDownCameraRot = { 1.57079f, 0.0f, 0.0f }; // 真下を向く回転 (90度)
    static inline const float kPlaneIntersectY = 0.0f;               // 地平面Y座標

    // マップ境界・スタート/ゴール判定用定数
    static inline const float kMapBoundaryX = 15.0f;                // マップの左右外枠 (X = -15 〜 15)
    static inline const float kMapBoundaryZ = 25.0f;                // マップの前後外枠 (Z = -25 〜 25)
    static inline const float kStartAreaZ = -20.0f;                 // スタートエリアの中心Z
    static inline const float kGoalAreaZ = 20.0f;                   // ゴールエリアの中心Z
    static inline const float kAreaRadius = 4.0f;                   // 円形判定エリアの半径
    static inline const int kCircleDivision = 32;                   // 円形ギズモの描画分割数
    static inline const float kPi = 3.14159265f;                    // 円周率

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

    // ルート関連メンバ変数
    GameMode mode_ = GameMode::DrawRoute;
    std::vector<Vector3> rawPoints_;                 // 記録した軌跡の点配列
    std::vector<Vector3> pathPoints_;                // 補間された滑らかなルート配列
    std::vector<float> accumDistances_;              // 各補間点の累積距離テーブル（等速化用）
    float currentDistance_ = 0.0f;                   // 現在の走行距離
    float totalDistance_ = 0.0f;                     // ルートの総距離
    std::vector<std::unique_ptr<LineObject>> lineObjects_; // 描画用ラインオブジェクト配列
    bool isDrawing_ = false;                         // 描画中フラグ
    bool hasReachedGoal_ = false;                    // ゴールエリア到達フラグ

    // 仮マップオブジェクトおよびギズモ
    std::vector<std::unique_ptr<CubeObject>> mapObjects_;          // 仮マップの柱
    std::vector<std::unique_ptr<LineObject>> editorGizmoLines_;    // マップ枠・スタート/ゴール枠表示ライン

    // 移動速度定数
    static inline const float kPlayerSpeed = 0.05f;  // 毎フレームの自機前進距離

    // AABB衝突判定関数
    bool IsCollidingAABB(const Vector3& pos1, const Vector3& size1, const Vector3& pos2, const Vector3& size2) const;

    // プレイモードのゲーム開始処理
    void StartGame();

    // シェイク機能用変数と定数
    int shakeTimer_ = 0;
    static inline const Vector3 kDefaultCameraPos = { 0.0f, 4.0f, -20.0f }; // メインカメラの基準位置
    static inline const int kShakeDuration = 15;                             // シェイクフレーム数
    static inline const float kShakeIntensity = 0.2f;                        // シェイクの強さ
    static inline const std::string kPlayerBulletHitEffectName = "YellowFire"; // プレイヤー弾ヒット時のエフェクト名（黄色炎）
    static inline const std::string kEnemyBulletHitEffectName = "PurpleFire"; // 敵弾ヒット時のエフェクト名（紫色炎）
    static inline const std::string kRainEffectName = "WaterDrop";            // 雨のエフェクト名
};

