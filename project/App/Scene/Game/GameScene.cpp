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
#include "Engine/Graphics/Objects/3d/Line/LineObject.h"

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
    for (float z = -480.0f; z <= 480.0f; z += 10.0f) {
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
    floorSquare_->SetSize({25.0f, 1000.0f});
	floorSquare_->SetRotate({ 1.57f, 0.0f, 0.0f }); // X軸で90度回転して水平にする
    floorSquare_->SetColor({ 0.5f, 1.0f, 0.5f, 1.0f }); // 黄緑色

    // 9. 柱の警告リングギズモ（描画モード用）の生成
    pillarGizmoLines_.clear();
    static const int kGizmoCircleDivision = 16; // 分割数を16にし描画負荷を低減
    static const float kGizmoRadius = 4.0f;     // 柱にぶつからないための安全警告半径
    static const float kPiVal = 3.14159265f;
    static const float kGizmoThickness = 0.2f;
    static const Vector4 kWarningColor = { 1.0f, 0.3f, 0.0f, 1.0f }; // オレンジの警告色

    for (const auto& pillar : mapObjects_) {
        Vector3 center = pillar->GetPosition();
        std::vector<Vector3> points;
        points.reserve(kGizmoCircleDivision);
        for (int i = 0; i < kGizmoCircleDivision; ++i) {
            float theta = (2.0f * kPiVal * static_cast<float>(i)) / static_cast<float>(kGizmoCircleDivision);
            float x = center.x + kGizmoRadius * std::cos(theta);
            float z = center.z + kGizmoRadius * std::sin(theta);
            points.push_back({ x, 0.01f, z });
        }

        for (int i = 0; i < kGizmoCircleDivision; ++i) {
            auto line = std::make_unique<LineObject>();
            line->Initialize(0);
            line->SetStartPoint(points[i]);
            line->SetEndPoint(points[(i + 1) % kGizmoCircleDivision]);
            line->SetThickness(kGizmoThickness);
            line->SetColor(kWarningColor);
            pillarGizmoLines_.push_back(std::move(line));
        }
    }
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
        // カメラの高さYを530.0fとし、Zは現在のエリアのZ中心にする
        static const float kDrawCameraHeight = 530.0f;
        static const float kHalfScale = 0.5f;
        float areaCenterZ = (route_->GetCurrentAreaStartZ() + route_->GetCurrentAreaGoalZ()) * kHalfScale;
        mainCamera_->SetPosition({ 0.0f, kDrawCameraHeight, areaCenterZ });
        mainCamera_->SetRotation(kTopDownCameraRot);
        mainCamera_->DisableTarget();

        // ルートエディタの更新
        route_->Update(mainCamera_.get());

        // 仮マップオブジェクト（柱Cube）の更新
        for (auto& pillar : mapObjects_) {
            pillar->Update();
        }

        // 柱の警告リングギズモの更新
        for (auto& line : pillarGizmoLines_) {
            line->Update();
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

        currentDistance_ += Player::GetAutoSpeed();
        if (currentDistance_ >= route_->GetTotalDistance()) {
            int currentArea = route_->GetCurrentAreaIndex();
            static const int kMaxAreaIndex = 3;

            if (currentArea < kMaxAreaIndex) {
                // 次のエリアへ移行
                mode_ = GameMode::DrawRoute;
                player_->SetAutoMoving(false);
                route_->ClearForNewArea();
                route_->SetupArea(currentArea + 1);
                currentDistance_ = 0.0f;
                return;
            } else {
                // 最終エリアのゴール到達時にクリアシーンへ
                SceneManager::GetInstance()->ChangeScene(kClearSceneName);
                return;
            }
        }

        // A/Dキーによるカメラ首振り制御
        if (input_->Press(DIK_A)) {
            cameraYawOffset_ -= kCameraYawSpeed;
        } else if (input_->Press(DIK_D)) {
            cameraYawOffset_ += kCameraYawSpeed;
        } else {
            // キーを離した際は正面（0.0f）に徐々に戻す
            if (cameraYawOffset_ > 0.0f) {
                cameraYawOffset_ -= kCameraYawReturnSpeed;
                if (cameraYawOffset_ < 0.0f) {
                    cameraYawOffset_ = 0.0f;
                }
            } else if (cameraYawOffset_ < 0.0f) {
                cameraYawOffset_ += kCameraYawReturnSpeed;
                if (cameraYawOffset_ > 0.0f) {
                    cameraYawOffset_ = 0.0f;
                }
            }
        }
        // 限界角にクランプ
        if (cameraYawOffset_ > kCameraYawLimit) {
            cameraYawOffset_ = kCameraYawLimit;
        } else if (cameraYawOffset_ < -kCameraYawLimit) {
            cameraYawOffset_ = -kCameraYawLimit;
        }

        // Routeクラスから現在の位置・接線方向・回転を取得
        Vector3 playerPos = route_->GetPositionAtDistance(currentDistance_);
        Vector3 tangent = route_->GetTangentAtDistance(currentDistance_);
        Vector3 rot = route_->GetRotationAtDistance(currentDistance_);

        player_->SetPosition(playerPos);
        player_->SetRotation(rot);
        player_->SetDirection(tangent);

        // 敵の動的湧き・ボス戦湧き処理 (isEnemyEnabled_ 時のみ)
        if (isEnemyEnabled_) {
            // 1. エディタトリガーによる湧き判定
            for (auto& trigger : spawnTriggers_) {
                if (!trigger.triggered && playerPos.z >= trigger.z) {
                    trigger.triggered = true;

                    // 指定数（trigger.count）の敵を湧かせる
                    for (int i = 0; i < trigger.count; ++i) {
                        Vector3 rightVec = { tangent.z, 0.0f, -tangent.x };
                        // 複数湧き対応のため、位置をずらす
                        float spawnDistBack = -10.0f - static_cast<float>(i) * 3.0f;
                        float spawnDistSide = (i % 2 == 0 ? 10.0f : -10.0f) + (static_cast<float>(i / 2) * 1.5f);
                        
                        Vector3 spawnPos = Math::Add(
                            playerPos, 
                            Math::Add(
                                Math::Multiply(spawnDistBack, tangent),
                                Math::Multiply(spawnDistSide, rightVec)
                            )
                        );

                        auto enemy = std::make_unique<Enemy>();
                        enemy->Initialize();
                        enemy->SetSpawnPoint(false); // 実体化
                        enemy->SetPosition(spawnPos);
                        enemy->SetTargetPlayer(player_.get());
                        enemy->SetAiState(Enemy::AiState::Approach);
                        
                        enemies_.push_back(std::move(enemy));
                    }
                }
            }

            // 2. 通常の定期的な敵の湧き判定 (120.0f ごと)
            if (playerPos.z - lastSpawnZ_ >= kSpawnIntervalZ) {
                lastSpawnZ_ = playerPos.z;
                
                Vector3 rightVec = { tangent.z, 0.0f, -tangent.x };
                static const float kSpawnDistBack = -10.0f;
                static const float kSpawnDistSide = 10.0f;
                
                Vector3 spawnPos = Math::Add(
                    playerPos, 
                    Math::Add(
                        Math::Multiply(kSpawnDistBack, tangent),
                        Math::Multiply((rand() % 2 == 0 ? kSpawnDistSide : -kSpawnDistSide), rightVec)
                    )
                );

                auto enemy = std::make_unique<Enemy>();
                enemy->Initialize();
                enemy->SetSpawnPoint(false);
                enemy->SetPosition(spawnPos);
                enemy->SetTargetPlayer(player_.get());
                enemy->SetAiState(Enemy::AiState::Approach);
                
                enemies_.push_back(std::move(enemy));
            }

            // 3. ボス戦の湧き判定 (エリア3のZ=360f以降に1回だけ)
            if (playerPos.z >= kBossSpawnZ && !hasBossSpawned_) {
                hasBossSpawned_ = true;

                // ボスをプレイヤーの正面少し先から出現させる
                Vector3 bossSpawnPos = Math::Add(playerPos, Math::Multiply(20.0f, tangent));
                bossSpawnPos.x = 0.0f; // 正面中央

                auto boss = std::make_unique<Enemy>();
                boss->Initialize();
                boss->SetBoss(true);
                boss->SetPosition(bossSpawnPos);
                boss->SetTargetPlayer(player_.get());
                boss->SetAiState(Enemy::AiState::Approach);

                enemies_.push_back(std::move(boss));
            }
        }

        // カメラ位置・注視点の計算（一人称視点）
        const float kCameraUpHeight = 1.8f;      // 目の高さのYオフセット
        const float kCameraLookAhead = 8.0f;     // 前方への注視点オフセット

        Vector3 camPos = Math::Add(playerPos, Vector3{ 0.0f, kCameraUpHeight, 0.0f });

        // 首振りを適用した注視方向ベクトルの計算
        float cosTheta = std::cos(cameraYawOffset_);
        float sinTheta = std::sin(cameraYawOffset_);
        Vector3 lookTangent;
        lookTangent.x = tangent.x * cosTheta + tangent.z * sinTheta;
        lookTangent.y = tangent.y; // Y軸回転なので上下は変更なし
        lookTangent.z = -tangent.x * sinTheta + tangent.z * cosTheta;
        lookTangent = Math::Normalize(lookTangent);

        Vector3 lookAtTarget = Math::Add(camPos, Math::Multiply(kCameraLookAhead, lookTangent));

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

    // 敵 (Enemy) の更新と死後消滅判定 (カリング適用)
    if (isEnemyEnabled_) {
        Vector3 pPos = player_->GetPosition();
        for (auto it = enemies_.begin(); it != enemies_.end();) {
            float distZ = std::abs((*it)->GetPosition().z - pPos.z);
            static const float kCullingDistance = 60.0f;
            if (distZ > kCullingDistance) {
                // はるか後方に置き去りにされた敵は、追いつけないため消滅させてリソース節約
                if ((*it)->GetPosition().z < pPos.z - kCullingDistance) {
                    it = enemies_.erase(it);
                } else {
                    ++it;
                }
                continue;
            }

            (*it)->SetTargetPlayer(player_.get()); // 射撃の誘導用にプレイヤーポインタを渡す
            (*it)->Update();
            if ((*it)->IsDead()) {
                it = enemies_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // ステージエディタの更新（ギズモとの同期）
    stageEditor_->Update();

    if (mode_ == GameMode::Play) {
        // 自機（プレイヤー）の被弾判定
        Vector3 playerPos = player_->GetPosition();
        Vector3 playerSize = player_->GetSize();
        AABB playerAABB;
        static constexpr float kHalf = 0.5f;
        playerAABB.min = { playerPos.x - playerSize.x * kHalf, playerPos.y - playerSize.y * kHalf, playerPos.z - playerSize.z * kHalf };
        playerAABB.max = { playerPos.x + playerSize.x * kHalf, playerPos.y + playerSize.y * kHalf, playerPos.z + playerSize.z * kHalf };

        if (isEnemyEnabled_) {
            for (auto& enemy : enemies_) {
                const auto& enemyBullets = enemy->GetBullets();
                for (auto& bullet : enemyBullets) {
                    if (!bullet->IsActive()) continue;

                    // 敵の弾を球体と見なして当たり判定
                    Sphere bulletSphere;
                    bulletSphere.center = bullet->GetPosition();
                    static constexpr float kBulletCollisionRadius = 0.5f;
                    bulletSphere.radius = kBulletCollisionRadius;

                    if (IsCollision(playerAABB, bulletSphere)) {
                        static constexpr int kEnemyDamage = 10;
                        player_->Damage(kEnemyDamage, bullet->GetEffectName());
                        bullet->Kill(); // 被弾した弾を非アクティブ化

                        // 被弾時にカメラシェイク
                        shakeTimer_ = kShakeDuration;
                    }
                }
            }
        }

        // プレイヤーの死亡（ゲームオーバー）判定
        if (player_->IsDead()) {
            SceneManager::GetInstance()->ChangeScene(kGameOverSceneName);
            return;
        }

        // プレイヤーの弾と敵の衝突判定 (即時レイキャストから物理弾判定へ置き換え)
        if (isEnemyEnabled_) {
            const auto& playerBullets = player_->GetBullets();
            for (auto& bullet : playerBullets) {
                if (!bullet->IsActive()) continue;

                Sphere bulletSphere;
                bulletSphere.center = bullet->GetPosition();
                static constexpr float kBulletCollisionRadius = 0.4f;
                bulletSphere.radius = kBulletCollisionRadius;

                for (auto& enemy : enemies_) {
                    if (enemy->IsSpawnPoint() || enemy->IsDead()) continue;

                    AABB headAABB = enemy->GetHeadCollider()->GetWorldAABB();
                    AABB bodyAABB = enemy->GetBodyCollider()->GetWorldAABB();

                    // 頭部（クリティカル）優先
                    if (IsCollision(headAABB, bulletSphere)) {
                        static constexpr int kCriticalDamage = 2;
                        enemy->Damage(kCriticalDamage, player_->GetBulletEffectName());
                        bullet->Kill(); // 弾を非アクティブ化
                        shakeTimer_ = kShakeDuration;
                        break;
                    }
                    // 胴体判定
                    else if (IsCollision(bodyAABB, bulletSphere)) {
                        static constexpr int kNormalDamage = 1;
                        enemy->Damage(kNormalDamage, player_->GetBulletEffectName());
                        bullet->Kill();
                        shakeTimer_ = kShakeDuration;
                        break;
                    }
                }
            }
        }

        // レティクルの更新 (マウスカーソルの位置に追従。ゲームビューの拡縮に対応)
        Vector2 viewSize = GameViewWindow::GetGameViewSize();
        Vector2 mousePos = GameViewWindow::GetMousePosition();
        
        float scaleX = static_cast<float>(WindowApp::kClientWidth) / viewSize.x;
        float scaleY = static_cast<float>(WindowApp::kClientHeight) / viewSize.y;
        Vector2 scaledMousePos = { mousePos.x * scaleX, mousePos.y * scaleY };

        reticleSprite_->SetPosition({ scaledMousePos.x - 64.0f, scaledMousePos.y - 64.0f, 0.0f });
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

    // 仮マップオブジェクト（柱Cube）の描画 (プレイモード時のみ60.0fカリング)
    Vector3 playerPos = player_->GetPosition();
    static const float kCullingDistance = 60.0f;
    for (auto& pillar : mapObjects_) {
        if (mode_ == GameMode::Play) {
            float distZ = std::abs(pillar->GetPosition().z - playerPos.z);
            if (distZ > kCullingDistance) {
                continue;
            }
        }
        pillar->Draw();
    }

    if (mode_ == GameMode::DrawRoute) {
        for (auto& line : pillarGizmoLines_) {
            line->Draw();
        }
        route_->Draw();
        return; // 描画モード時はキャラやエフェクトを描画しない
    }

    // プレイヤー（およびプレイヤーの弾）の描画
    player_->Draw();

    // 複数敵の描画 (60.0fカリング)
    if (isEnemyEnabled_) {
        Vector3 playerPos = player_->GetPosition();
        static const float kCullingDistance = 60.0f;
        for (auto& enemy : enemies_) {
            float distZ = std::abs(enemy->GetPosition().z - playerPos.z);
            if (distZ <= kCullingDistance) {
                enemy->Draw();
            }
        }
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
    Vector3 startPlayerPos = route_->GetPositionAtDistance(0.0f);
    player_->SetPosition(startPlayerPos);

    // 4. 動的湧きデータの初期設定 (最初のエリア開始時のみ一括構築)
    if (route_->GetCurrentAreaIndex() == 0) {
        spawnTriggers_.clear();
        for (const auto& enemy : enemies_) {
            if (enemy->IsSpawnPoint()) {
                SpawnTrigger trigger;
                trigger.z = enemy->GetPosition().z;
                trigger.count = static_cast<int>(enemy->GetSize().x);
                if (trigger.count < 1) trigger.count = 1;
                if (trigger.count > 5) trigger.count = 5;
                trigger.triggered = false;
                spawnTriggers_.push_back(trigger);
            }
        }
        enemies_.clear(); // エディタ用サークル（ダミー）をクリアして動的湧きにする
        hasBossSpawned_ = false;
    }
    lastSpawnZ_ = startPlayerPos.z;

    // プレイ開始時にカメラ位置と回転をプレイ用の位置にリセット
    static const float kCameraStartUpHeight = 1.8f;      // 目の高さのYオフセット
    static const float kCameraStartLookAhead = 8.0f;     // 前方への注視点オフセット
    Vector3 startTangent = route_->GetTangentAtDistance(0.0f);

    Vector3 camPos = Math::Add(startPlayerPos, Vector3{ 0.0f, kCameraStartUpHeight, 0.0f });
    Vector3 lookAtTarget = Math::Add(camPos, Math::Multiply(kCameraStartLookAhead, startTangent));

    mainCamera_->SetPosition(camPos);
    mainCamera_->SetTarget(lookAtTarget);
    mainCamera_->SetRotation({ 0.2f, 0.0f, 0.0f });
}
