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
#include "App/Scene/Game/Stage/Stage.h"

// 不要になったヒットエフェクト名定数を削除

/**
 * @brief ゲーム本編シーンの初期化処理
 * カメラ、ライト、Player、Enemyオブジェクトの生成と初期パラメータ設定を行います。
 */
void GameScene::Initialize() {
    // 0. ポストプロセスのポインタを取得してメンバ変数に保持
    postProcess_ = SceneManager::GetInstance()->GetPostProcess();
    if (postProcess_) {
        //postProcess_->SetUnderwaterActive(true);
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

    // ズームカメラの生成とマネージャへの登録（ルート初期化後に配置）
    cameraZoom_ = std::make_shared<BaseCamera>();
    cameraZoom_->Initialize();
    float startZ = route_->GetCurrentAreaStartZ();
    targetZoom_ = { 0.0f, 0.0f, startZ };
    cameraZoom_->SetPosition(Math::Add(targetZoom_, kZoomCameraOffset));
    cameraZoom_->SetTarget(targetZoom_);
    cameraMgr_->AddCamera("Zoom", cameraZoom_);

    // 視野可視化用枠線の初期化（緑のライン）
    zoomFrameLines_.clear();
    for (int i = 0; i < 4; ++i) {
        auto line = std::make_unique<LineObject>();
        line->Initialize(0);
        static const float kFrameLineThickness = 0.3f;
        line->SetThickness(kFrameLineThickness);
        line->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f });
        zoomFrameLines_.push_back(std::move(line));
    }

    stageEditor_ = std::make_unique<StageEditor>();
    stageEditor_->Initialize(&enemies_);

    // 保存されたステージがあればロードする
    stageEditor_->LoadStage("resources/stages/stage1.json");

    // 7. マップステージの初期化
    stage_ = std::make_unique<Stage>();
    stage_->Initialize();

    // 2Dミニマップ用の背景スプライトの初期化
    minimapBg_ = std::make_unique<SpriteObject>();
    minimapBg_->Initialize(0); // ライティングなし
    minimapBg_->GetMaterialData()->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白背景

    // スタート・ゴール・カーソル赤丸インジケータ (2D)
    startIcon_ = std::make_unique<SpriteObject>();
    startIcon_->Initialize(0);
    startIcon_->GetMaterialData()->color = { 0.1f, 0.8f, 0.1f, 1.0f }; // 緑色（白背景で見やすい色）

    goalIcon_ = std::make_unique<SpriteObject>();
    goalIcon_->Initialize(0);
    goalIcon_->GetMaterialData()->color = { 0.0f, 0.5f, 1.0f, 1.0f }; // 青色

    indicatorIcon_ = std::make_unique<SpriteObject>();
    indicatorIcon_->Initialize(0);
    indicatorIcon_->GetMaterialData()->color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤色

    // 柱アイコンの初期化
    pillarIcons_.clear();
    const auto& pillars = stage_->GetPillars();
    for (size_t i = 0; i < pillars.size(); ++i) {
        auto icon = std::make_unique<SpriteObject>();
        icon->Initialize(0);
        icon->GetMaterialData()->color = { 0.9f, 0.2f, 0.2f, 1.0f }; // 赤っぽい色
        pillarIcons_.push_back(std::move(icon));
    }

    // 右画面用 3D 赤丸インジケータ (ズーム3D空間用)
    cursorIndicatorZoom_ = std::make_unique<SphereObject>();
    cursorIndicatorZoom_->Initialize();
    cursorIndicatorZoom_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
    cursorIndicatorZoom_->SetScale({ 1.2f, 1.2f, 1.2f });

    // 視野範囲枠線スプライトの初期化 (2D)
    for (int i = 0; i < 4; ++i) {
        zoomFrame2D_[i] = std::make_unique<SpriteObject>();
        zoomFrame2D_[i]->Initialize(0);
        zoomFrame2D_[i]->GetMaterialData()->color = { 0.0f, 0.8f, 0.0f, 1.0f }; // 緑色
    }

    // ミニマップ外枠線スプライトの初期化 (2D)
    for (int i = 0; i < 4; ++i) {
        minimapBorderFrame2D_[i] = std::make_unique<SpriteObject>();
        minimapBorderFrame2D_[i]->Initialize(0);
        minimapBorderFrame2D_[i]->GetMaterialData()->color = { 0.5f, 0.5f, 0.5f, 1.0f }; // 中間グレー（白背景で見やすい）
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
        // 1. 固定のDirectX12バックバッファ解像度を取得
        float clientW = static_cast<float>(WindowApp::kClientWidth);
        float clientH = static_cast<float>(WindowApp::kClientHeight);

        // ImGuiのGame Viewウィンドウのサイズを取得し、マウス座標を1280x720スケールに変換する
        Vector2 mousePos = GameViewWindow::GetMousePosition();
        Vector2 viewSize = GameViewWindow::GetGameViewSize();
        Vector2 scaledMousePos = mousePos;
        if (viewSize.x > 0.0f && viewSize.y > 0.0f) {
            scaledMousePos.x = (mousePos.x / viewSize.x) * clientW;
            scaledMousePos.y = (mousePos.y / viewSize.y) * clientH;
        }

        float vpWidth = clientW * 0.3f;
        float vpHeight = clientH;

        // マップのアスペクト比フィット計算 (臨機応変に太く表示するため 0.45f に変更)
        static const float kAspect3D = 0.45f;
        float aspect2D = vpWidth / vpHeight;

        float mapW = 0.0f;
        float mapH = 0.0f;
        if (aspect2D > kAspect3D) {
            // ビューポートが横長 -> 高さフィット
            mapH = vpHeight;
            mapW = mapH * kAspect3D;
        } else {
            // ビューポートが縦長 -> 幅フィット
            mapW = vpWidth;
            mapH = mapW / kAspect3D;
        }

        float offsetX = (vpWidth - mapW) * 0.5f;
        float offsetY = (vpHeight - mapH) * 0.5f;

        float startZ = route_->GetCurrentAreaStartZ();
        float goalZ = route_->GetCurrentAreaGoalZ();

        // 2. 左画面マウス位置からの2D->3D座標逆変換（ドラッグによるルート描画）
        bool isClickStarted = input_->MouseTrigger(0);
        bool isPressing = input_->MousePress(0);

        // 手書き中であるか、またはミニマップ白枠内でクリックが開始された場合
        if (isPressing && (route_->IsDrawing() || (isClickStarted && scaledMousePos.x <= vpWidth))) {
            float marginX = mapW * 0.1f;
            float tX = (scaledMousePos.x - (offsetX + marginX)) / (mapW - 2.0f * marginX);
            tX = std::clamp(tX, 0.0f, 1.0f); // 枠外にはみ出しても境界クランプでドラッグを継続

            float marginY = mapH * 0.1f;
            float tZ = ((offsetY + mapH - marginY) - scaledMousePos.y) / (mapH - 2.0f * marginY);
            tZ = std::clamp(tZ, 0.0f, 1.0f); // 枠外にはみ出しても境界クランプでドラッグを継続

            Vector3 dragWorldPos;
            dragWorldPos.x = -15.0f + tX * 30.0f;
            dragWorldPos.z = startZ + tZ * (goalZ - startZ);
            dragWorldPos.y = 0.0f;

            // ★追加: マウス手ブレ防止用ローパスフィルタ（平滑化）
            static Vector3 s_smoothedDragPos = dragWorldPos;
            if (isClickStarted) {
                s_smoothedDragPos = dragWorldPos;
            } else {
                s_smoothedDragPos.x = s_smoothedDragPos.x * 0.6f + dragWorldPos.x * 0.4f;
                s_smoothedDragPos.y = s_smoothedDragPos.y * 0.6f + dragWorldPos.y * 0.4f;
                s_smoothedDragPos.z = s_smoothedDragPos.z * 0.6f + dragWorldPos.z * 0.4f;
            }

            // 2D入力でルートを更新
            route_->Update2D(s_smoothedDragPos);

            // ★追加: 線を引いているときにカメラの注視点をその位置に即座に追従させる
            targetZoom_ = s_smoothedDragPos;
        } else {
            // マウスドラッグしていない時は描画停止処理
            route_->Update2D({ 9999.0f, 0.0f, 0.0f }); // 範囲外のダミー値で手書き停止を誘発
        }

        // ★追加: 右クリック（または右ドラッグ）によるカメラのLoL風ミニマップ移動
        if (input_->MousePress(1) && scaledMousePos.x <= vpWidth) {
            float marginX = mapW * 0.1f;
            float tX = (scaledMousePos.x - (offsetX + marginX)) / (mapW - 2.0f * marginX);
            tX = std::clamp(tX, 0.0f, 1.0f);

            float marginY = mapH * 0.1f;
            float tZ = ((offsetY + mapH - marginY) - scaledMousePos.y) / (mapH - 2.0f * marginY);
            tZ = std::clamp(tZ, 0.0f, 1.0f);

            targetZoom_.x = -15.0f + tX * 30.0f;
            targetZoom_.z = startZ + tZ * (goalZ - startZ);
        }

        // 3. ズームカメラの更新（右ドラッグによるスクロール等）
        UpdateZoomCamera();

        // 4. 右画面（Zoomカメラ 3D空間用）のWVP計算・更新
        cameraMgr_->SetActiveCamera("Zoom");
        cameraZoom_->UpdateProjection((clientW * 0.7f) / clientH); // 右70%用アスペクト比を設定
        stage_->Update();
        route_->UpdateSpheres();
        route_->UpdateLines();

        // 3D赤丸インジケータ（右画面の床用）の更新
        Vector3 indicatorPos = { targetZoom_.x, 0.1f, targetZoom_.z };
        cursorIndicatorZoom_->SetPosition(indicatorPos);
        cursorIndicatorZoom_->Update();

        // アクティブカメラをMainに戻しておく
        cameraMgr_->SetActiveCamera(kMainCameraName);

        // 5. 2Dミニマップ表示用スプライトの座標計算・更新
        // 一時的に2Dプロジェクション行列を左画面（vpWidth×vpHeight）に適合させる
        cameraMgr_->SetProjectionMatrix2D(Math::MakeOrthographicMatrix(0.0f, 0.0f, vpWidth, vpHeight, 0.0f, 100.0f));

        float marginX = mapW * 0.1f;
        float marginY = mapH * 0.1f;

        // 背景スプライトは左画面全体を白で埋める
        minimapBg_->SetSize(vpWidth, vpHeight);
        minimapBg_->SetPosition({ 0.0f, 0.0f });
        minimapBg_->Update();

        // スタートアイコン
        float startIconX = offsetX + marginX + 0.5f * (mapW - 2.0f * marginX); // X=0 (中央)
        float startIconY = offsetY + mapH - marginY; // Z=StartZ (一番手前)
        startIcon_->SetSize(24.0f, 24.0f);
        startIcon_->SetPosition({ startIconX - 12.0f, startIconY - 12.0f });
        startIcon_->Update();

        // ゴールアイコン
        float goalIconX = offsetX + marginX + 0.5f * (mapW - 2.0f * marginX); // X=0 (中央)
        float goalIconY = offsetY + marginY; // Z=GoalZ (一番奥)
        goalIcon_->SetSize(24.0f, 24.0f);
        goalIcon_->SetPosition({ goalIconX - 12.0f, goalIconY - 12.0f });
        goalIcon_->Update();

        // 柱アイコン
        const auto& pillars = stage_->GetPillars();
        for (size_t i = 0; i < pillars.size(); ++i) {
            Vector3 pPos = pillars[i]->GetPosition();
            float tX = (pPos.x - (-15.0f)) / 30.0f;
            float px = offsetX + marginX + tX * (mapW - 2.0f * marginX);

            float tZ = (pPos.z - startZ) / (goalZ - startZ);
            float py = (offsetY + mapH - marginY) - tZ * (mapH - 2.0f * marginY);

            // 柱サイズに応じた大きさに設定
            pillarIcons_[i]->SetSize(16.0f, 16.0f);
            pillarIcons_[i]->SetPosition({ px - 8.0f, py - 8.0f });
            pillarIcons_[i]->Update();
        }

        // 赤丸インジケータ（左画面のカーソル現在位置用）
        float indTX = (targetZoom_.x - (-15.0f)) / 30.0f;
        float indPX = offsetX + marginX + indTX * (mapW - 2.0f * marginX);
        float indTZ = (targetZoom_.z - startZ) / (goalZ - startZ);
        float indPY = (offsetY + mapH - marginY) - indTZ * (mapH - 2.0f * marginY);
        indicatorIcon_->SetSize(24.0f, 24.0f);
        indicatorIcon_->SetPosition({ indPX - 12.0f, indPY - 12.0f });
        indicatorIcon_->Update();

        // 手書きルート軌跡線の更新（プールによる高速使い回し＋Catmull-Rom補間による超滑らか描画！）
        const auto& rawPoints = route_->GetRawPoints();
        activeMiniMapLineCount_ = 0;
        if (rawPoints.size() >= 2) {
            // 1. 手書き点の平滑化（3回移動平均）
            std::vector<Vector3> smoothedPoints = rawPoints;
            if (smoothedPoints.size() >= 3) {
                for (int iter = 0; iter < 3; ++iter) {
                    std::vector<Vector3> temp = smoothedPoints;
                    for (size_t i = 1; i < smoothedPoints.size() - 1; ++i) {
                        temp[i].x = (smoothedPoints[i - 1].x + smoothedPoints[i].x + smoothedPoints[i + 1].x) / 3.0f;
                        temp[i].y = (smoothedPoints[i - 1].y + smoothedPoints[i].y + smoothedPoints[i + 1].y) / 3.0f;
                        temp[i].z = (smoothedPoints[i - 1].z + smoothedPoints[i].z + smoothedPoints[i + 1].z) / 3.0f;
                    }
                    smoothedPoints = temp;
                }
            }

            // 2. 平滑化した制御点から、Catmull-Rom補間で滑らかな高密度線分配列を生成
            std::vector<Vector3> densePoints = Math::GenerateCatmullRomPath(smoothedPoints, 5); // 各区間を5分割

            size_t neededLines = 0;
            if (densePoints.size() >= 2) {
                neededLines = densePoints.size() - 1;
            }

            // 不足分をプールに新規追加
            while (routeLineSprites_.size() < neededLines) {
                auto lineSprite = std::make_unique<SpriteObject>();
                lineSprite->Initialize(0);
                routeLineSprites_.push_back(std::move(lineSprite));
            }

            activeMiniMapLineCount_ = neededLines;

            for (size_t i = 0; i < neededLines; ++i) {
                Vector3 pt0 = densePoints[i];
                Vector3 pt1 = densePoints[i + 1];

                // 2Dミニマップ上の座標に変換
                float tX0 = (pt0.x - (-15.0f)) / 30.0f;
                float px0 = offsetX + marginX + tX0 * (mapW - 2.0f * marginX);
                float tZ0 = (pt0.z - startZ) / (goalZ - startZ);
                float py0 = (offsetY + mapH - marginY) - tZ0 * (mapH - 2.0f * marginY);

                float tX1 = (pt1.x - (-15.0f)) / 30.0f;
                float px1 = offsetX + marginX + tX1 * (mapW - 2.0f * marginX);
                float tZ1 = (pt1.z - startZ) / (goalZ - startZ);
                float py1 = (offsetY + mapH - marginY) - tZ1 * (mapH - 2.0f * marginY);

                float dx = px1 - px0;
                float dy = py1 - py0;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 0.001f) {
                    dist = 0.001f;
                }

                float angle = atan2f(dy, dx);

                // プール内の既存スプライトを再利用してパラメータ設定
                auto& lineSprite = routeLineSprites_[i];
                lineSprite->GetMaterialData()->color = { 0.0f, 0.0f, 0.0f, 1.0f }; // 黒色
                lineSprite->SetSize(dist, 1.2f); // ギザギザを減らしスマートに見せるため、太さを1.2fに細くする
                lineSprite->GetTransform().rotate.z = angle;
                lineSprite->SetPosition({ px0, py0 });
                lineSprite->Update();
            }
        }

        // ★追加: ズームカメラの視野範囲を表記する四角 (2D)
        // ズームカメラの視野サイズ定数（Y=24.0f の高さから見た地平面上の視野サイズ）
        static const float kZoomViewWidth3D = 24.0f;
        static const float kZoomViewHeight3D = 18.0f;

        float vpRightAspect = (clientW * 0.7f) / clientH;
        float viewH3D = kZoomViewHeight3D;
        float viewW3D = viewH3D * vpRightAspect;

        float minX3D = targetZoom_.x - viewW3D * 0.5f;
        float maxX3D = targetZoom_.x + viewW3D * 0.5f;
        float minZ3D = targetZoom_.z - viewH3D * 0.5f;
        float maxZ3D = targetZoom_.z + viewH3D * 0.5f;

        auto to2D = [&](float wx, float wz) -> Vector2 {
            float tX = (wx - (-15.0f)) / 30.0f;
            float px = offsetX + marginX + tX * (mapW - 2.0f * marginX);

            float tZ = (wz - startZ) / (goalZ - startZ);
            float py = (offsetY + mapH - marginY) - tZ * (mapH - 2.0f * marginY);
            return { px, py };
        };

        Vector2 pMin = to2D(minX3D, minZ3D);
        Vector2 pMax = to2D(maxX3D, maxZ3D);

        float x0 = pMin.x;
        float x1 = pMax.x;
        float y0 = pMax.y; // 2DではYが小さい方が上
        float y1 = pMin.y; // Yが大きい方が下
        float frameThickness = 2.0f;

        // 0: 上辺
        zoomFrame2D_[0]->SetPosition({ x0, y0 });
        zoomFrame2D_[0]->SetSize(x1 - x0, frameThickness);
        zoomFrame2D_[0]->Update();

        // 1: 下辺
        zoomFrame2D_[1]->SetPosition({ x0, y1 - frameThickness });
        zoomFrame2D_[1]->SetSize(x1 - x0, frameThickness);
        zoomFrame2D_[1]->Update();

        // 2: 左辺
        zoomFrame2D_[2]->SetPosition({ x0, y0 });
        zoomFrame2D_[2]->SetSize(frameThickness, y1 - y0);
        zoomFrame2D_[2]->Update();

        // 3: 右辺
        zoomFrame2D_[3]->SetPosition({ x1 - frameThickness, y0 });
        zoomFrame2D_[3]->SetSize(frameThickness, y1 - y0);
        zoomFrame2D_[3]->Update();

        // ★追加: ミニマップの有効サイズを示す四角い外枠 (2D)
        float borderThickness = 2.0f;
        float bx0 = offsetX + marginX;
        float bx1 = offsetX + mapW - marginX;
        float by0 = offsetY + marginY;
        float by1 = offsetY + mapH - marginY;

        // 0: 上辺
        minimapBorderFrame2D_[0]->SetPosition({ bx0, by0 });
        minimapBorderFrame2D_[0]->SetSize(bx1 - bx0, borderThickness);
        minimapBorderFrame2D_[0]->Update();

        // 1: 下辺
        minimapBorderFrame2D_[1]->SetPosition({ bx0, by1 - borderThickness });
        minimapBorderFrame2D_[1]->SetSize(bx1 - bx0, borderThickness);
        minimapBorderFrame2D_[1]->Update();

        // 2: 左辺
        minimapBorderFrame2D_[2]->SetPosition({ bx0, by0 });
        minimapBorderFrame2D_[2]->SetSize(borderThickness, by1 - by0);
        minimapBorderFrame2D_[2]->Update();

        // 3: 右辺
        minimapBorderFrame2D_[3]->SetPosition({ bx1 - borderThickness, by0 });
        minimapBorderFrame2D_[3]->SetSize(borderThickness, by1 - by0);
        minimapBorderFrame2D_[3]->Update();

        // 2Dプロジェクション行列を全画面解像度に復元する
        cameraMgr_->SetProjectionMatrix2D(Math::MakeOrthographicMatrix(0.0f, 0.0f, clientW, clientH, 0.0f, 100.0f));

        // ゴールまで到達している場合、SPACEキーでゲーム開始できるようにする
        if (route_->HasReachedGoal() && input_->Trigger(DIK_SPACE)) {
            StartGame();
        }

        dirLight_->Update();
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

                // ズームカメラの注視点を新しいエリアのスタート位置にリセット
                targetZoom_ = { 0.0f, 0.0f, route_->GetCurrentAreaStartZ() };
                cameraZoom_->SetPosition(Math::Add(targetZoom_, kZoomCameraOffset));
                cameraZoom_->SetTarget(targetZoom_);
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
        Vector3 targetLookTangent;
        targetLookTangent.x = tangent.x * cosTheta + tangent.z * sinTheta;
        targetLookTangent.y = tangent.y; // Y軸回転なので上下は変更なし
        targetLookTangent.z = -tangent.x * sinTheta + tangent.z * cosTheta;
        targetLookTangent = Math::Normalize(targetLookTangent);

        // ★追加: 前フレームの注視方向ベクトルと Lerp し、急激な首振りを平滑化する！
        static Vector3 s_prevLookTangent = targetLookTangent;
        if (currentDistance_ <= 0.1f) {
            s_prevLookTangent = targetLookTangent;
        } else {
            // 85%は前フレームを維持、15%だけ新しい方向を向く（ローパスフィルタ）
            s_prevLookTangent.x = s_prevLookTangent.x * 0.85f + targetLookTangent.x * 0.15f;
            s_prevLookTangent.y = s_prevLookTangent.y * 0.85f + targetLookTangent.y * 0.15f;
            s_prevLookTangent.z = s_prevLookTangent.z * 0.85f + targetLookTangent.z * 0.15f;
            s_prevLookTangent = Math::Normalize(s_prevLookTangent);
        }

        Vector3 lookAtTarget = Math::Add(camPos, Math::Multiply(kCameraLookAhead, s_prevLookTangent));

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

    // マップステージの更新
    stage_->Update();

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


}

/**
 * @brief 毎フレーム描画処理（3Dオブジェクトのレンダリングコマンド発行）
 */
void GameScene::Draw() {
    // マップステージの描画
    Vector3 playerPos = player_->GetPosition();
    bool showWarning = (mode_ == GameMode::DrawRoute);
    bool isPlayMode = (mode_ == GameMode::Play);

    if (mode_ == GameMode::DrawRoute) {
        // --- 画面分割（スプリットビュー）の描画 ---
        
        // 1. 固定のDirectX12バックバッファ解像度を取得
        float clientW = static_cast<float>(WindowApp::kClientWidth);
        float clientH = static_cast<float>(WindowApp::kClientHeight);

        // 2. ビューポート・シザーの定義
        // 左側 (30%)
        D3D12_VIEWPORT vpLeft{};
        vpLeft.Width = clientW * 0.3f;
        vpLeft.Height = clientH;
        vpLeft.TopLeftX = 0.0f;
        vpLeft.TopLeftY = 0.0f;
        vpLeft.MinDepth = 0.0f;
        vpLeft.MaxDepth = 1.0f;

        D3D12_RECT scLeft{};
        scLeft.left = 0;
        scLeft.right = static_cast<LONG>(vpLeft.Width);
        scLeft.top = 0;
        scLeft.bottom = static_cast<LONG>(clientH);

        // 右側 (70%)
        D3D12_VIEWPORT vpRight{};
        vpRight.Width = clientW * 0.7f;
        vpRight.Height = clientH;
        vpRight.TopLeftX = clientW * 0.3f;
        vpRight.TopLeftY = 0.0f;
        vpRight.MinDepth = 0.0f;
        vpRight.MaxDepth = 1.0f;

        D3D12_RECT scRight{};
        scRight.left = static_cast<LONG>(vpRight.TopLeftX);
        scRight.right = static_cast<LONG>(clientW);
        scRight.top = 0;
        scRight.bottom = static_cast<LONG>(clientH);

        auto dxCommon = Zuizui::GetInstance()->GetDxCommon();
        auto commandList = dxCommon->GetCommandList();

        // 3. 左画面の描画（2Dミニマップ）
        commandList->RSSetViewports(1, &vpLeft);
        commandList->RSSetScissorRects(1, &scLeft);

        // 2Dスプライトの描画
        minimapBg_->Draw("white");

        for (auto& icon : pillarIcons_) {
            icon->Draw("white");
        }

        startIcon_->Draw("circle_solid");
        goalIcon_->Draw("circle_solid");

        // 有効な線分のみ描画
        for (size_t i = 0; i < activeMiniMapLineCount_; ++i) {
            routeLineSprites_[i]->Draw("white");
        }

        indicatorIcon_->Draw("circle_solid");

        // ズームカメラ視野範囲の枠線を描画 (2D)
        for (int i = 0; i < 4; ++i) {
            zoomFrame2D_[i]->Draw("white");
        }

        // ミニマップの外枠線を描画 (2D)
        for (int i = 0; i < 4; ++i) {
            minimapBorderFrame2D_[i]->Draw("white");
        }

        // 4. 右画面の描画（Zoomカメラ 3Dビュー）
        cameraZoom_->UpdateProjection(vpRight.Width / vpRight.Height);
        cameraMgr_->SetActiveCamera("Zoom");

        // 右画面のビューポートを設定して描画
        commandList->RSSetViewports(1, &vpRight);
        commandList->RSSetScissorRects(1, &scRight);

        stage_->Draw(showWarning, isPlayMode, playerPos);
        route_->Draw();
        cursorIndicatorZoom_->Draw();

        return; // 描画モード時はキャラやエフェクトを描画しない
    }

    // プレイモードの通常描画
    stage_->Draw(showWarning, isPlayMode, playerPos);

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
        reticleSprite_->Update(); // ★重要: 描画直前にカメラの確定したビュー・プロジェクションで行列を再計算し遅延を完璧にゼロにする
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
    mainCamera_->Update(); // ★追加！開始直後のカメラ表示バグを防ぐため行列を即時更新する
    cameraMgr_->SetActiveCamera("Main"); // ★追加！アクティブカメラをプレイ用のMainカメラに設定する
}

void GameScene::UpdateZoomCamera() {
    Vector2 mousePos = GameViewWindow::GetMousePosition();
    Vector2 viewSize = GameViewWindow::GetGameViewSize();
    float clientW = static_cast<float>(WindowApp::kClientWidth);
    float clientH = static_cast<float>(WindowApp::kClientHeight);
    
    Vector2 scaledMousePos = mousePos;
    if (viewSize.x > 0.0f && viewSize.y > 0.0f) {
        scaledMousePos.x = (mousePos.x / viewSize.x) * clientW;
        scaledMousePos.y = (mousePos.y / viewSize.y) * clientH;
    }
    
    float vpWidth = clientW * 0.3f;

    // 右クリックかつマウスが右側の3Dビューポートにある場合は、既存の自由スクロールを残す
    if (input_->MousePress(1) && scaledMousePos.x > vpWidth) {
        float dx = input_->GetMouseDeltaX();
        float dy = input_->GetMouseDeltaY();

        targetZoom_.x += dx * kZoomScrollSpeed;
        targetZoom_.z -= dy * kZoomScrollSpeed;

        static const float kMapLimitX = 15.0f;
        targetZoom_.x = std::clamp(targetZoom_.x, -kMapLimitX, kMapLimitX);
        targetZoom_.z = std::clamp(targetZoom_.z, route_->GetCurrentAreaStartZ(), route_->GetCurrentAreaGoalZ());
    }

    // ズームカメラの位置と注視点を反映
    cameraZoom_->SetPosition(Math::Add(targetZoom_, kZoomCameraOffset));
    cameraZoom_->SetTarget(targetZoom_);
    cameraZoom_->Update();
}

GameScene::GameScene() = default;
GameScene::~GameScene() = default;
