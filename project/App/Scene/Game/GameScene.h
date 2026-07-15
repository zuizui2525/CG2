#pragma once
#include "App/Scene/Core/IScene.h"
#include <memory>
#include <string>

class PostProcess;
class LineObject;
class Stage;

// エンジンコンポーネントのインクルード
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"
#include "App/Scene/Game/Player.h"
#include "App/Scene/Game/Enemy.h"
#include "App/Scene/Game/Route.h"
#include "App/Scene/Game/StageEditor.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"
#include "Engine/Graphics/Objects/3d/Square/SquareObject.h"
#include "Engine/Graphics/Objects/2d/Sprite/SpriteObject.h"

/**
 * @brief ゲーム本編を管理するシーンクラス
 */
class GameScene : public IScene {
public:
    // コンストラクタ・デストラクタ
    GameScene();
    ~GameScene() override;

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
    static inline const Vector3 kTopDownCameraPos = { 0.0f, 120.0f, 0.0f }; // 上空見下ろしカメラ位置（スタート・ゴールが画面内に確実に収まる高さ）
    static inline const Vector3 kTopDownCameraRot = { 1.57079f, 0.0f, 0.0f }; // 真下を向く回転 (90度)

private:
    // エンジンの各種マネージャへの生ポインタ（所有権は持たない）
    Input* input_ = nullptr;            // キー入力マネージャ
    CameraManager* cameraMgr_ = nullptr; // カメラ管理者
    LightManager* lightMgr_ = nullptr;  // ライト管理者
    PostProcess* postProcess_ = nullptr;

    // シーン内で作成・管理するオブジェクト
    std::shared_ptr<BaseCamera> mainCamera_;        // メインカメラ
    std::shared_ptr<BaseCamera> cameraZoom_;        // ズームカメラ（拡大3Dビュー用）
    std::shared_ptr<DebugCamera> debugCamera_;      // デバッグ確認用フリーカメラ
    Vector3 targetZoom_ = { 0.0f, 0.0f, 0.0f };      // ズームカメラの注視点
    std::vector<std::unique_ptr<LineObject>> zoomFrameLines_; // 左画面に描画するカメラ視野可視化枠線
    void UpdateZoomCamera();                        // ズームカメラの更新制御関数
    std::unique_ptr<DirectionalLightObject> dirLight_; // 平行光源
    std::unique_ptr<Player> player_;                // プレイヤーオブジェクト
    std::vector<std::unique_ptr<Enemy>> enemies_;   // 複数敵オブジェクトリスト
    std::unique_ptr<SpriteObject> reticleSprite_;   // 照準UIスプライト
    bool showRouteEditor_ = true;                   // Route Editorウィンドウの表示フラグ

    // リファクタリングによる新設コンポーネント
    std::unique_ptr<Route> route_;
    std::unique_ptr<StageEditor> stageEditor_;

    // ルート関連メンバ変数
    GameMode mode_ = GameMode::DrawRoute;
    float currentDistance_ = 0.0f;                   // 現在の走行距離

    // マップステージ
    std::unique_ptr<Stage> stage_;

    // 2Dミニマップ用のメンバ変数 (左画面 2D 描画用)
    std::unique_ptr<SpriteObject> minimapBg_;
    std::unique_ptr<SpriteObject> startIcon_;
    std::unique_ptr<SpriteObject> goalIcon_;
    std::unique_ptr<SpriteObject> indicatorIcon_;
    std::vector<std::unique_ptr<SpriteObject>> pillarIcons_;
    std::vector<std::unique_ptr<SpriteObject>> routeLineSprites_;
    std::unique_ptr<SpriteObject> zoomFrame2D_[4];   // ズームカメラの視野範囲枠線 (2D)
    std::unique_ptr<SpriteObject> minimapBorderFrame2D_[4]; // ミニマップの外枠線 (2D)
    size_t activeMiniMapLineCount_ = 0;              // 有効なルート手書き線スプライト数



    // 右画面用 3D インジケータ (ズームカメラ 3D 空間用)
    std::unique_ptr<SphereObject> cursorIndicatorZoom_;

    struct SpawnTrigger {
        float z;
        int count;
        bool triggered;
    };
    std::vector<SpawnTrigger> spawnTriggers_;

    // AABB衝突判定関数
    bool IsCollidingAABB(const Vector3& pos1, const Vector3& size1, const Vector3& pos2, const Vector3& size2) const;

    // プレイモードのゲーム開始処理
    void StartGame();

public:
    // エディタ表示フラグのポインタ取得ゲッター
    bool* GetShowStageEditorPtr() { return stageEditor_->GetShowEditorPtr(); }
    bool* GetShowRouteEditorPtr() { return &showRouteEditor_; }
private:

    // シェイク機能用変数と定数
    bool isEnemyEnabled_ = true; // 敵の有効化フラグ（不要になったら削除可能）
    int shakeTimer_ = 0;
    float cameraYawOffset_ = 0.0f; // A/Dキーでのカメラ首振りヨー角オフセット
    float lastSpawnZ_ = -240.0f;   // 前回の敵の湧きZ座標
    bool hasBossSpawned_ = false;  // ボスがすでに湧いたか
    static inline const Vector3 kDefaultCameraPos = { 0.0f, 4.0f, -20.0f }; // メインカメラの基準位置
    static inline const Vector3 kZoomCameraOffset = { 0.0f, 25.0f, -20.0f }; // ズームカメラの注視点からのオフセット
    static inline const float kZoomScrollSpeed = 0.15f;                      // ズームカメラの右ドラッグスクロール速度
    static inline const int kShakeDuration = 15;                             // シェイクフレーム数
    static inline const float kShakeIntensity = 0.2f;                        // シェイクの強さ
    static inline const std::string kRainEffectName = "WaterDrop";            // 雨のエフェクト名

    // カメラ首振り用定数
    static inline const float kCameraYawLimit = 0.35f;                       // 最大首振りヨー角（ラジアン）
    static inline const float kCameraYawSpeed = 0.015f;                      // 首振り速度
    static inline const float kCameraYawReturnSpeed = 0.02f;                 // 正面復帰速度

    // 敵湧き制御用定数
    static inline const float kSpawnIntervalZ = 15.0f;                      // 敵の湧くZ間隔
    static inline const float kBossSpawnZ = 180.0f;                          // ボスが湧くZ座標
};

