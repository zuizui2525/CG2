#include "App/Scene/Game/GameScene.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Base/WindowApp/WindowApp.h"
#include "Engine/Math/Collision/Collision.h"
#include <algorithm>
#include <fstream>
#include "Engine/Zuizui.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Graphics/PostProcess/PostProcess.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectFactory.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include "externals/imgui/imgui.h"
#include "Engine/Debug/GameViewWindow.h"
#include "Engine/Debug/SceneHierarchy.h"
#include <cstdlib>

// 不要になったヒットエフェクト名定数を削除

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
    mainCamera_->SetPosition(kTopDownCameraPos);
    mainCamera_->SetRotation(kTopDownCameraRot);
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

    // 5. エフェクトシステムの初期化と全エフェクト登録
    auto effectMgr = EffectManager::GetInstance();
    effectMgr->Initialize();
    EffectFactory::GetInstance()->RegisterAllEffects();

    // 雨のエフェクト再生開始（ループ）
    EffectPlayParam rainParam;
    rainParam.isLoop = true;
    rainParam.position = { 0.0f, 0.0f, 0.0f };
    rainParam.scale = { 1.0f, 1.0f, 1.0f };
    effectMgr->PlayEffect3D(kRainEffectName, rainParam);

    // 6. プレイヤーおよび敵オブジェクトの生成と初期化
    player_ = std::make_unique<Player>();
    player_->Initialize();

    enemies_.clear();

    reticleSprite_ = std::make_unique<SpriteObject>();
    reticleSprite_->Initialize(0); // ライティングなし
    reticleSprite_->SetSize(128.0f, 128.0f);

    // ルートとステージエディタの生成と初期化
    route_ = std::make_unique<Route>();
    route_->Initialize(input_, cameraMgr_);

    stageEditor_ = std::make_unique<StageEditor>();
    stageEditor_->Initialize(&enemies_);

    // 保存されたステージがあればロードする
    stageEditor_->LoadStage("resources/stages/stage1.json");

    // 7. 仮マップオブジェクト (交互に並ぶ柱Cube) の生成
    mapObjects_.clear();
    for (float z = -20.0f; z <= 20.0f; z += 10.0f) {
        float x = (static_cast<int>(z) % 20 == 0) ? -12.0f : 12.0f;
        auto pillar = std::make_unique<CubeObject>();
        pillar->Initialize();
        pillar->SetPosition({ x, 5.0f, z });
        pillar->SetScale({ 2.0f, 10.0f, 2.0f });
        pillar->SetColor({ 0.6f, 0.6f, 0.7f, 1.0f });
        mapObjects_.push_back(std::move(pillar));
    }

	// 8. 地面用の平面オブジェクトの生成
	floorSquare_ = std::make_unique<SquareObject>();
    floorSquare_->Initialize();
	floorSquare_->SetPosition({ 0.0f, 0.0f, 0.0f });
    floorSquare_->SetSize({25.0f, 50.0f});
	floorSquare_->SetRotate({ 1.57f, 0.0f, 0.0f }); // X軸で90度回転して水平にする
    floorSquare_->SetColor({ 0.5f, 1.0f, 0.5f, 1.0f }); // 黄緑色
}

/**
 * @brief ImGuiによるデバッグ表示処理
 */
void GameScene::ImGuiControl() {
#ifdef _USEIMGUI
    if (showRouteEditor_ && mode_ == GameMode::DrawRoute) {
        ImGui::Begin("Route Editor");
        ImGui::Text("Mouse drag to draw route on ground.");
        ImGui::Text("Has Reached Goal: %s", route_->HasReachedGoal() ? "Yes" : "No");
        
        if (route_->HasReachedGoal()) {
            if (ImGui::Button("Start Game")) {
                StartGame();
            }
        } else {
            ImGui::TextDisabled("Drag from yellow start sphere to blue goal sphere.");
        }
        ImGui::End();
    }

    stageEditor_->ImGuiControl();

    // カメラ切り替え等のマネージャパラメータを表示
    cameraMgr_->ImGuiControl();

    // ポストプロセスのパラメータ調整用ImGuiコントロール
    if (postProcess_) {
        postProcess_->ImGuiControl();
    }

    // エフェクトのパラメータ調整用ImGuiコントロール
    EffectManager::GetInstance()->ImGuiControl("Effects");
#endif
}

/**
 * @brief 毎フレーム更新処理（キー入力によるカメラ切り替え・オブジェクトの座標・パラメータ更新）
 */
void GameScene::Update() {
    // ルート描画モードの更新処理
    if (mode_ == GameMode::DrawRoute) {
        // 毎フレームカメラの位置・回転とTarget無効化を強制適用（エディタ等の上書き防止）
        mainCamera_->SetPosition(kTopDownCameraPos);
        mainCamera_->SetRotation(kTopDownCameraRot);
        mainCamera_->DisableTarget();

        // ルートエディタの更新
        route_->Update(mainCamera_.get());

        // 仮マップオブジェクト（柱Cube）の更新
        for (auto& pillar : mapObjects_) {
            pillar->Update();
        }

        // ゴールまで到達している場合、SPACEキーでゲーム開始できるようにする
        if (route_->HasReachedGoal() && input_->Trigger(DIK_SPACE)) {
            StartGame();
        }

        dirLight_->Update();
        mainCamera_->Update();
        return;
    }

#ifdef _USEIMGUI
    // TABキーによりメインカメラとデバッグカメラを切り替える
    static constexpr int kCameraToggleKey = DIK_TAB; // カメラ切り替え用キー定数
    if (input_->Trigger(kCameraToggleKey)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? kMainCameraName : kDebugCameraName);
    }
#endif

    // プレイモード中の自動走行処理
    if (mode_ == GameMode::Play) {
        player_->SetAutoMoving(true);

        currentDistance_ += kPlayerSpeed;
        if (currentDistance_ >= route_->GetTotalDistance()) {
            // ゴール到達時にクリアシーンへ
            SceneManager::GetInstance()->ChangeScene(kClearSceneName);
            return;
        }

        // Routeクラスから現在の位置・接線方向・回転を取得
        Vector3 playerPos = route_->GetPositionAtDistance(currentDistance_);
        Vector3 tangent = route_->GetTangentAtDistance(currentDistance_);
        Vector3 rot = route_->GetRotationAtDistance(currentDistance_);

        player_->SetPosition(playerPos);
        player_->SetRotation(rot);
        player_->SetDirection(tangent);

        // カメラ位置・注視点の計算（一人称視点）
        const float kCameraUpHeight = 1.8f;      // 目の高さのYオフセット
        const float kCameraLookAhead = 8.0f;     // 前方への注視点オフセット

        Vector3 camPos = Math::Add(playerPos, Vector3{ 0.0f, kCameraUpHeight, 0.0f });
        Vector3 lookAtTarget = Math::Add(camPos, Math::Multiply(kCameraLookAhead, tangent));

        mainCamera_->SetPosition(camPos);
        mainCamera_->SetTarget(lookAtTarget);
        mainCamera_->Update(); // デバッグカメラ起動中も武器位置を正しく同期するため、メインカメラの行列を強制的に更新する

        // スタート/ゴール球体の位置・ビュー行列計算の更新
        route_->UpdateSpheres();
    }

    // プレイモード中は武器の位置と角度を更新する
    if (mode_ == GameMode::Play) {
        player_->UpdateWeapon(mainCamera_->GetViewMatrix());
    }

    // 雨のエフェクトの位置をメインカメラの位置に追従させる（走行中も常に自分の周りに雨を降らせ、エディタカメラの移動には影響されないようにする）
    if (auto rainEffect = EffectManager::GetInstance()->GetEffect(kRainEffectName)) {
        rainEffect->GetTransform().translate = mainCamera_->GetPosition();
    }

    // プレイヤーの更新 (自動走行位置同期後に呼び出すことで、弾の発射などが連動する)
    player_->Update();

    // 敵 (Enemy) の更新と死後消滅判定
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        (*it)->Update();
        if ((*it)->IsDead()) {
            it = enemies_.erase(it);
        } else {
            ++it;
        }
    }

    // ステージエディタの更新（ギズモとの同期）
    stageEditor_->Update();

    if (mode_ == GameMode::Play) {
        // プレイヤーの即時射撃（レイキャスト）判定
        if (player_->HasFiredThisFrame()) {
            Vector2 viewSize = GameViewWindow::GetGameViewSize();
            Vector2 mouseCenter = { viewSize.x * 0.5f, viewSize.y * 0.5f };
            Vector3 rayStart, rayDir;
            mainCamera_->CreateRay(mouseCenter, viewSize.x, viewSize.y, rayStart, rayDir);

            Segment raySegment;
            raySegment.origin = rayStart;
            raySegment.diff = Math::Multiply(100.0f, rayDir); // 射程 100.0f

            Enemy* hitEnemy = nullptr;
            float minDistance = FLT_MAX;
            for (auto& enemy : enemies_) {
                Vector3 pos = enemy->GetPosition();
                Vector3 size = enemy->GetSize();
                
                AABB aabb;
                aabb.min = { pos.x - size.x * 0.5f, pos.y - size.y * 0.5f, pos.z - size.z * 0.5f };
                aabb.max = { pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, pos.z + size.z * 0.5f };

                if (IsCollision(aabb, raySegment)) {
                    float dist = Math::Length(Math::Subtract(pos, rayStart));
                    if (dist < minDistance) {
                        minDistance = dist;
                        hitEnemy = enemy.get();
                    }
                }
            }

            if (hitEnemy) {
                // 10ダメージを与えて即時破壊（最大HP=10）
                hitEnemy->Damage(10, kPlayerBulletHitEffectName);
                
                // ヒット時にカメラシェイク
                shakeTimer_ = kShakeDuration;
            }
        }

        // レティクルの更新 (解像度 1280x720 基準の2Dプロジェクション空間における中央に配置)
        float centerX = static_cast<float>(WindowApp::kClientWidth);
        float centerY = static_cast<float>(WindowApp::kClientHeight);
        reticleSprite_->SetPosition({ (centerX - 128.0f) * 0.5f, (centerY - 128.0f) * 0.5f, 0.0f });
        reticleSprite_->Update();
    }

    // 仮マップオブジェクト（柱Cube）の更新
    for (auto& pillar : mapObjects_) {
        pillar->Update();
    }

    // エフェクトの更新
    EffectManager::GetInstance()->Update();

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

        // カメラシェイク更新
        Vector3 camPos = mainCamera_->GetPosition();
        if (shakeTimer_ > 0) {
            shakeTimer_--;
            float rx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * kShakeIntensity;
            float ry = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * kShakeIntensity;
            float rz = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * kShakeIntensity;
            camPos += Vector3{ rx, ry, rz };
            mainCamera_->SetPosition(camPos);
        }

        activeCamera->Update();
    }

	// 床の更新
    floorSquare_->Update();
}

/**
 * @brief 毎フレーム描画処理（3Dオブジェクトのレンダリングコマンド発行）
 */
void GameScene::Draw() {
    // 床の描画
    if (mode_ == GameMode::Play) {
        floorSquare_->Draw();
    }

    // 仮マップオブジェクト（柱Cube）の描画
    for (auto& pillar : mapObjects_) {
        pillar->Draw();
    }

    if (mode_ == GameMode::DrawRoute) {
        route_->Draw();
        return; // 描画モード時はキャラやエフェクトを描画しない
    }

    // プレイヤー（およびプレイヤーの弾）の描画
    player_->Draw();

    // 複数敵の描画
    for (auto& enemy : enemies_) {
        enemy->Draw();
    }

    // エフェクトの描画
    EffectManager::GetInstance()->Draw();

    // プレイモード中のみレティクル（照準）およびスタート/ゴール球体の描画
    if (mode_ == GameMode::Play) {
        route_->DrawSpheres();
        reticleSprite_->Draw("reticle");
    }
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

/**
 * @brief プレイモードのゲーム開始処理
 */
void GameScene::StartGame() {
    // 1. ルートの確定処理
    route_->FinalizeRoute();
    
    currentDistance_ = 0.0f;

    // 2. モードをプレイに変更
    mode_ = GameMode::Play;
    
    // 3. 自機(Player)の位置をルートの始点に設定
    player_->SetPosition(route_->GetPositionAtDistance(0.0f));

    // プレイ開始時にカメラ位置と回転をプレイ用の位置にリセット
    mainCamera_->SetPosition(kDefaultCameraPos);
    mainCamera_->SetRotation({ 0.2f, 0.0f, 0.0f });
}
