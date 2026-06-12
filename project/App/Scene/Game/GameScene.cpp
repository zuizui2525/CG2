#include "App/Scene/Game/GameScene.h"
#include "Engine/Base/BaseResource.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Graphics/PostProcess/PostProcess.h"

/**
 * @brief ゲーム本編シーンの初期化処理
 * カメラ、ライト、Player、Enemyオブジェクトの生成と初期パラメータ設定を行います。
 */
void GameScene::Initialize() {
    // 0. ポストプロセスのポインタを取得してメンバ変数に保持
    postProcess_ = SceneManager::GetInstance()->GetPostProcess();
    if (postProcess_) {
        postProcess_->SetUnderwaterActive(true);
    }

    // 1. 各マネージャへのポインタをリソース管理者から取得
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // 2. メインカメラの生成とマネージャへの登録
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
	mainCamera_->SetPosition({ 0.0f, 4.0f, -20.0f });
	mainCamera_->SetRotation({ 0.2f, 0.0f, 0.0f });
    cameraMgr_->AddCamera(kMainCameraName, mainCamera_);

    // 3. デバッグカメラの生成とマネージャへの登録
    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera(kDebugCameraName, debugCamera_);

    // アプリ起動直後はメインカメラをアクティブ状態に設定する
    cameraMgr_->SetActiveCamera(kMainCameraName);

    // 4. 平行光源の生成、初期化、マネージャへの追加
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    // 5. プレイヤーおよび敵オブジェクトの生成と初期化
    player_ = std::make_unique<Player>();
    player_->Initialize();

    enemy_ = std::make_unique<Enemy>();
    enemy_->Initialize();
}

/**
 * @brief ImGuiによるデバッグ表示処理
 */
void GameScene::ImGuiControl() {
#ifdef _USEIMGUI
    // カメラ切り替え等のマネージャパラメータを表示
    cameraMgr_->ImGuiControl();

    // 平行光源パラメータの調整用ImGuiコントロール
    static const char* kLightImGuiLabel = "GameScene DirLight";
    dirLight_->ImGuiControl(kLightImGuiLabel);

    // ポストプロセスのパラメータ調整用ImGuiコントロール
    if (postProcess_) {
        postProcess_->ImGuiControl();
    }
#endif
}

/**
 * @brief 毎フレーム更新処理（キー入力によるカメラ切り替え、オブジェクトの座標・パラメータ更新）
 */
void GameScene::Update() {
#ifdef _USEIMGUI
    // TABキーによりメインカメラとデバッグカメラを切り替える
    static constexpr int kCameraToggleKey = DIK_TAB; // カメラ切り替え用キー定数
    if (input_->Trigger(kCameraToggleKey)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? kMainCameraName : kDebugCameraName);
    }
#endif

    // プレイヤーと敵の更新
    player_->Update();
    enemy_->Update();

    // 衝突判定（3D AABB）
    // 1. プレイヤーの弾と敵本体
    const auto& playerBullets = player_->GetBullets();
    for (const auto& bullet : playerBullets) {
        if (bullet->IsActive()) {
            if (IsCollidingAABB(bullet->GetPosition(), bullet->GetSize(), enemy_->GetPosition(), enemy_->GetSize())) {
                bullet->Kill();
                SceneManager::GetInstance()->ChangeScene(kClearSceneName);
                break;
            }
        }
    }

    // 2. 敵の弾とプレイヤー本体
    const auto& enemyBullets = enemy_->GetBullets();
    for (const auto& bullet : enemyBullets) {
        if (bullet->IsActive()) {
            if (IsCollidingAABB(bullet->GetPosition(), bullet->GetSize(), player_->GetPosition(), player_->GetSize())) {
                bullet->Kill();
                SceneManager::GetInstance()->ChangeScene(kGameOverSceneName);
                break;
            }
        }
    }

    // ライトパラメータの行列更新
    dirLight_->Update();

    // 現在アクティブなカメラを判定し、それぞれのカメラ種別に応じた更新処理を呼ぶ
    BaseCamera* activeCamera = cameraMgr_->GetActiveCamera();
    DebugCamera* debugCamPtr = dynamic_cast<DebugCamera*>(activeCamera);

    if (debugCamPtr) {
        // デバッグカメラ有効化とキー操作による移動更新
        debugCamPtr->SetActive(true);
        debugCamPtr->Update(input_);
    } else {
        // 通常のメインカメラの更新処理
        debugCamera_->SetActive(false);
        activeCamera->Update();
    }
}

/**
 * @brief 毎フレーム描画処理（3Dオブジェクトのレンダリングコマンド発行）
 */
void GameScene::Draw() {
    // プレイヤー（およびプレイヤーの弾）の描画
    player_->Draw();

    // 敵（および敵の弾）の描画
    enemy_->Draw();
}

/**
 * @brief 3D AABBによる衝突判定
 */
bool GameScene::IsCollidingAABB(const Vector3& pos1, const Vector3& size1, const Vector3& pos2, const Vector3& size2) const {
    static constexpr float kHalf = 0.5f;

    float minX1 = pos1.x - size1.x * kHalf;
    float maxX1 = pos1.x + size1.x * kHalf;
    float minY1 = pos1.y - size1.y * kHalf;
    float maxY1 = pos1.y + size1.y * kHalf;
    float minZ1 = pos1.z - size1.z * kHalf;
    float maxZ1 = pos1.z + size1.z * kHalf;

    float minX2 = pos2.x - size2.x * kHalf;
    float maxX2 = pos2.x + size2.x * kHalf;
    float minY2 = pos2.y - size2.y * kHalf;
    float maxY2 = pos2.y + size2.y * kHalf;
    float minZ2 = pos2.z - size2.z * kHalf;
    float maxZ2 = pos2.z + size2.z * kHalf;

    return (minX1 <= maxX2 && maxX1 >= minX2) &&
           (minY1 <= maxY2 && maxY1 >= minY2) &&
           (minZ1 <= maxZ2 && maxZ1 >= minZ2);
}
