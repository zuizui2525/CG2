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

    // 保存されたステージがあればロードする
    LoadStage(kStageFilePath);

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

    // 8. マップ外枠・スタート/ゴール枠のGizmoLine生成
    editorGizmoLines_.clear();
    auto AddGizmoRect = [this](const Vector3& center, float width, float depth, const Vector4& color) {
        float hx = width * 0.5f;
        float hz = depth * 0.5f;
        
        Vector3 corners[4] = {
            { center.x - hx, 0.01f, center.z - hz },
            { center.x + hx, 0.01f, center.z - hz },
            { center.x + hx, 0.01f, center.z + hz },
            { center.x - hx, 0.01f, center.z + hz }
        };
        
        for (int i = 0; i < 4; ++i) {
            auto line = std::make_unique<LineObject>();
            line->Initialize(0);
            line->SetStartPoint(corners[i]);
            line->SetEndPoint(corners[(i + 1) % 4]);
            const float kGizmoThickness = 0.15f;
            line->SetThickness(kGizmoThickness);
            line->SetColor(color);
            editorGizmoLines_.push_back(std::move(line));
        }
    };

    auto AddGizmoCircle = [this](const Vector3& center, float radius, const Vector4& color) {
        std::vector<Vector3> points;
        points.reserve(kCircleDivision);
        for (int i = 0; i < kCircleDivision; ++i) {
            float theta = (2.0f * kPi * i) / kCircleDivision;
            float x = center.x + radius * std::cos(theta);
            float z = center.z + radius * std::sin(theta);
            points.push_back({ x, 0.01f, z });
        }

        for (int i = 0; i < kCircleDivision; ++i) {
            auto line = std::make_unique<LineObject>();
            line->Initialize(0);
            line->SetStartPoint(points[i]);
            line->SetEndPoint(points[(i + 1) % kCircleDivision]);
            const float kGizmoThickness = 0.15f;
            line->SetThickness(kGizmoThickness);
            line->SetColor(color);
            editorGizmoLines_.push_back(std::move(line));
        }
    };

    // マップ外枠 (白)
    AddGizmoRect({ 0.0f, 0.0f, 0.0f }, kMapBoundaryX * 2.0f, kMapBoundaryZ * 2.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
    // スタート枠 (黄・円形)
    AddGizmoCircle({ 0.0f, 0.0f, kStartAreaZ }, kAreaRadius, { 1.0f, 1.0f, 0.0f, 1.0f });
    // ゴール枠 (青・円形)
    AddGizmoCircle({ 0.0f, 0.0f, kGoalAreaZ }, kAreaRadius, { 0.0f, 0.5f, 1.0f, 1.0f });
    // 9. スタート地点とゴール地点の視覚用球体オブジェクトの生成
    startSphere_ = std::make_unique<SphereObject>();
    startSphere_->Initialize();
    startSphere_->SetPosition({ 0.0f, 1.0f, kStartAreaZ });
    startSphere_->SetScale({ kAreaRadius * 2.0f, kAreaRadius * 2.0f, kAreaRadius * 2.0f });
    startSphere_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f }); // 黄色（半透明）

    goalSphere_ = std::make_unique<SphereObject>();
    goalSphere_->Initialize();
    goalSphere_->SetPosition({ 0.0f, 1.0f, kGoalAreaZ });
    goalSphere_->SetScale({ kAreaRadius * 2.0f, kAreaRadius * 2.0f, kAreaRadius * 2.0f });
    goalSphere_->SetColor({ 0.0f, 0.5f, 1.0f, 0.5f }); // 青色（半透明）

	// 10. 地面用の平面オブジェクトの生成
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
        ImGui::Text("Points: %d", (int)rawPoints_.size());
        
        // 4点以上あればゲーム開始可能にする
        const size_t kMinPointsToStart = 4;
        if (rawPoints_.size() >= kMinPointsToStart) {
            if (ImGui::Button("Start Game")) {
                StartGame();
            }
        } else {
            ImGui::TextDisabled("Need at least 4 points to start.");
        }
        ImGui::End();
    }

    if (showStageEditor_) {
        // ステージエディタウィンドウ (ImGui)
        ImGui::Begin("Stage Editor");
        if (ImGui::Button("Add Enemy")) {
            auto enemy = std::make_unique<Enemy>();
            enemy->Initialize();
            enemy->SetPosition({ 0.0f, 1.0f, 0.0f });
            enemies_.push_back(std::move(enemy));
            selectedEnemyIndex_ = (int)enemies_.size() - 1;
            // ギズモのターゲットに設定
            SceneHierarchy::GetInstance()->SetSelected(enemies_.back()->GetCube());
        }

        ImGui::Separator();
        ImGui::Text("Enemies List:");
        for (int i = 0; i < (int)enemies_.size(); ++i) {
            std::string label = "Enemy " + std::to_string(i);
            bool isSelected = (selectedEnemyIndex_ == i);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedEnemyIndex_ = i;
                SceneHierarchy::GetInstance()->SetSelected(enemies_[i]->GetCube());
            }
        }

        if (selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < (int)enemies_.size()) {
            ImGui::Separator();
            ImGui::Text("Selected Enemy Transform:");
            auto& enemy = enemies_[selectedEnemyIndex_];
            Vector3 pos = enemy->GetPosition();
            Vector3 size = enemy->GetSize();
            
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
                enemy->SetPosition(pos);
            }
            if (ImGui::DragFloat3("Scale (Size)", &size.x, 0.1f, 0.1f, 10.0f)) {
                enemy->SetSize(size);
            }
            
            if (ImGui::Button("Delete Enemy")) {
                // 選択中の敵を削除
                if (SceneHierarchy::GetInstance()->GetSelected() == enemy->GetCube()) {
                    SceneHierarchy::GetInstance()->SetSelected(nullptr);
                }
                enemies_.erase(enemies_.begin() + selectedEnemyIndex_);
                selectedEnemyIndex_ = -1;
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Save Stage")) {
            SaveStage(kStageFilePath);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Stage")) {
            LoadStage(kStageFilePath);
        }
        ImGui::End();
    }

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

        // マウスの左ボタン押下状態
        const int kLeftMouseButton = 0;
        if (input_->MousePress(kLeftMouseButton)) {
            if (GameViewWindow::IsMouseOnGameView()) {
                Vector2 mousePos = GameViewWindow::GetMousePosition();
                Vector2 viewSize = GameViewWindow::GetGameViewSize();

                Vector3 rayStart, rayDir;
                mainCamera_->CreateRay(mousePos, viewSize.x, viewSize.y, rayStart, rayDir);

                // 地平面 Y = kPlaneIntersectY との交差判定
                const float kZeroThreshold = 0.0001f;
                if (std::abs(rayDir.y) > kZeroThreshold) {
                    float t = (kPlaneIntersectY - rayStart.y) / rayDir.y;
                    if (t >= 0.0f) {
                        Vector3 intersectPos = rayStart + t * rayDir;

                        if (!isDrawing_) {
                            // 1. スタートエリア内からドラッグを開始したかチェック（ゴール後でも再描画可能）
                            float diffX = intersectPos.x - 0.0f;
                            float diffZ = intersectPos.z - kStartAreaZ;
                            float distSq = diffX * diffX + diffZ * diffZ;
                            if (distSq <= kAreaRadius * kAreaRadius) {
                                // 新規ドラッグ開始時に以前の線をクリアする
                                rawPoints_.clear();
                                lineObjects_.clear();
                                hasReachedGoal_ = false;

                                isDrawing_ = true;
                                rawPoints_.push_back(intersectPos);
                            }
                        } else {
                            // まだゴールに到達していない場合のみ軌跡を更新
                            if (!hasReachedGoal_) {
                                Vector3 diff = Math::Subtract(intersectPos, rawPoints_.back());
                                float dist = Math::Length(diff);
                                if (dist >= kMinPointDistance) {
                                    // 2. ゴールエリアに到達したかチェック
                                    float toGoalX = intersectPos.x - 0.0f;
                                    float toGoalZ = intersectPos.z - kGoalAreaZ;
                                    float distToGoalSq = toGoalX * toGoalX + toGoalZ * toGoalZ;

                                    rawPoints_.push_back(intersectPos);
                                    
                                    auto line = std::make_unique<LineObject>();
                                    line->Initialize(0);
                                    line->SetStartPoint(rawPoints_[rawPoints_.size() - 2]);
                                    line->SetEndPoint(rawPoints_.back());
                                    const float kLineThickness = 0.15f;
                                    line->SetThickness(kLineThickness);
                                    lineObjects_.push_back(std::move(line));

                                    // ゴールエリア内に入ったらドラッグ自動終了
                                    if (distToGoalSq <= kAreaRadius * kAreaRadius) {
                                        hasReachedGoal_ = true;
                                        isDrawing_ = false;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                isDrawing_ = false;
            }
        } else {
            isDrawing_ = false;
        }

        // ラインの更新
        for (auto& line : lineObjects_) {
            line->Update();
        }

        // ギズモライン（マップ外枠・スタート/ゴール矩形）の更新
        for (auto& line : editorGizmoLines_) {
            line->Update();
        }

        // 仮マップオブジェクト（柱Cube）の更新
        for (auto& pillar : mapObjects_) {
            pillar->Update();
        }

        // ゴールまで到達している場合、SPACEキーでゲーム開始できるようにする
        if (hasReachedGoal_ && input_->Trigger(DIK_SPACE)) {
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
        if (currentDistance_ >= totalDistance_) {
            // ゴール到達時にクリアシーンへ
            SceneManager::GetInstance()->ChangeScene(kClearSceneName);
            return;
        }

        // 累積距離テーブルから現在のインデックスと補間率を算出
        size_t idx = 0;
        for (size_t i = 0; i < accumDistances_.size() - 1; ++i) {
            if (accumDistances_[i] <= currentDistance_ && currentDistance_ < accumDistances_[i + 1]) {
                idx = i;
                break;
            }
        }

        float tLocal = 0.0f;
        float distDiff = accumDistances_[idx + 1] - accumDistances_[idx];
        if (distDiff > 0.0001f) {
            tLocal = (currentDistance_ - accumDistances_[idx]) / distDiff;
        }

        // 座標の線形補間
        Vector3 playerPos = Math::Add(pathPoints_[idx], Math::Multiply(tLocal, Math::Subtract(pathPoints_[idx + 1], pathPoints_[idx])));
        player_->SetPosition(playerPos);

        // 接線方向（向き）の算出
        Vector3 tangent = Math::Normalize(Math::Subtract(pathPoints_[idx + 1], pathPoints_[idx]));

        // 回転の算出（ピッチ・ヨー）
        float yaw = std::atan2(tangent.x, tangent.z);
        float pitch = -std::atan2(tangent.y, std::sqrt(tangent.x * tangent.x + tangent.z * tangent.z));
        player_->SetRotation({ pitch, yaw, 0.0f });
        player_->SetDirection(tangent);

        // カメラ位置・注視点の計算（一人称視点）
        const float kCameraUpHeight = 1.8f;      // 目の高さのYオフセット
        const float kCameraLookAhead = 8.0f;     // 前方への注視点オフセット

        Vector3 camPos = Math::Add(playerPos, Vector3{ 0.0f, kCameraUpHeight, 0.0f });
        Vector3 lookAtTarget = Math::Add(camPos, Math::Multiply(kCameraLookAhead, tangent));

        mainCamera_->SetPosition(camPos);
        mainCamera_->SetTarget(lookAtTarget);
        mainCamera_->Update(); // デバッグカメラ起動中も武器位置を正しく同期するため、メインカメラの行列を強制的に更新する
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

    // スタート・ゴール球体の更新
    startSphere_->Update();
    goalSphere_->Update();

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
        // マップ外枠・スタート/ゴール枠のデバッグ用補助枠線描画
        for (auto& line : editorGizmoLines_) {
            line->Draw();
        }
        // 描画モード時のみ手書きルート線を描画する
        for (const auto& line : lineObjects_) {
            line->Draw();
        }
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

    // プレイモード中のみスタート・ゴール地点の球体を描画する
    if (mode_ == GameMode::Play) {
        startSphere_->Draw();
        goalSphere_->Draw();

        // レティクル（照準）の描画
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
    // 1. Catmull-Rom補間による滑らかなルートを生成
    const int kPathDivision = 20;
    pathPoints_ = Math::GenerateCatmullRomPath(rawPoints_, kPathDivision);
    
    // 2. 累積距離テーブルの構築（等速化用）
    accumDistances_.clear();
    accumDistances_.push_back(0.0f);
    float accum = 0.0f;
    for (size_t i = 1; i < pathPoints_.size(); ++i) {
        float dist = Math::Length(Math::Subtract(pathPoints_[i], pathPoints_[i - 1]));
        accum += dist;
        accumDistances_.push_back(accum);
    }
    totalDistance_ = accum;
    currentDistance_ = 0.0f;

    // 3. モードをプレイに変更
    mode_ = GameMode::Play;
    
    // 4. 自機(Player)の位置をルートの始点に設定
    if (!pathPoints_.empty()) {
        player_->SetPosition(pathPoints_.front());
    }

    // プレイ開始時にカメラ位置と回転をプレイ用の位置にリセット
    mainCamera_->SetPosition(kDefaultCameraPos);
    mainCamera_->SetRotation({ 0.2f, 0.0f, 0.0f });
}

void GameScene::SaveStage(const std::string& filepath) {
    CreateDirectoryA("resources", NULL);
    CreateDirectoryA("resources/stages", NULL);

    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        return;
    }

    ofs << "[\n";
    for (size_t i = 0; i < enemies_.size(); ++i) {
        auto& enemy = enemies_[i];
        Vector3 pos = enemy->GetPosition();
        Vector3 size = enemy->GetSize();
        ofs << "  {\"pos\": [" << pos.x << ", " << pos.y << ", " << pos.z << "], "
            << "\"size\": [" << size.x << ", " << size.y << ", " << size.z << "]}";
        if (i + 1 < enemies_.size()) {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "]\n";
}

void GameScene::LoadStage(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        return;
    }

    enemies_.clear();
    selectedEnemyIndex_ = -1;

    std::string line;
    while (std::getline(ifs, line)) {
        size_t posIdx = line.find("\"pos\": [");
        if (posIdx == std::string::npos) continue;

        size_t posEnd = line.find("]", posIdx);
        if (posEnd == std::string::npos) continue;

        std::string posStr = line.substr(posIdx + 8, posEnd - (posIdx + 8));
        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (sscanf_s(posStr.c_str(), "%f, %f, %f", &px, &py, &pz) != 3) continue;

        size_t sizeIdx = line.find("\"size\": [");
        float sx = 1.0f, sy = 1.0f, sz = 1.0f;
        if (sizeIdx != std::string::npos) {
            size_t sizeEnd = line.find("]", sizeIdx);
            if (sizeEnd != std::string::npos) {
                std::string sizeStr = line.substr(sizeIdx + 9, sizeEnd - (sizeIdx + 9));
                sscanf_s(sizeStr.c_str(), "%f, %f, %f", &sx, &sy, &sz);
            }
        }

        auto enemy = std::make_unique<Enemy>();
        enemy->Initialize();
        enemy->SetPosition({ px, py, pz });
        enemy->SetSize({ sx, sy, sz });
        enemies_.push_back(std::move(enemy));
    }
}
