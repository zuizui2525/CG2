#include "App/App.h"
#include "App/Scene/Core/SceneManager.h"
#include "App/Scene/Core/SceneFactory.h"
#include "App/Load/ResourceLoader.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"

// アスペクト比・解像度変更用列挙型（マジックナンバー排除）
enum class AspectType {
    Aspect16_9_Low,
    Aspect16_9_High,
    Aspect4_3,
    Aspect1_1
};

void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
#ifdef _USEIMGUI
    engine_->Initialize(L"ZuizuiEngine");
#else
    engine_->Initialize(L"LE3B_02_イトウカズイ");
#endif
    EngineResource::SetEngine(engine_);

    input_ = std::make_unique<Input>();
    input_->Initialize();
    InputResource::SetInput(input_.get());

    cameraMgr_ = std::make_unique<CameraManager>();
    cameraMgr_->Initialize();
    CameraResource::SetCameraManager(cameraMgr_.get());

    lightMgr_ = std::make_unique<LightManager>();
    lightMgr_->Initialize();
    LightResource::SetLightManager(lightMgr_.get());

    texMgr_ = std::make_unique<TextureManager>();
    texMgr_->Initialize();
    TextureResource::SetTextureManager(texMgr_.get());

    modelMgr_ = std::make_unique<ModelManager>();
    modelMgr_->Initialize();
    ModelResource::SetModelManager(modelMgr_.get());

    // リソースの一括ロード
    ResourceLoader::LoadAll();

    sceneFactory_ = std::make_unique<SceneFactory>();
    SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
    SceneManager::GetInstance()->ChangeScene("Debug");

    // --- PostProcess の初期化 ---
    postProcess_ = std::make_unique<PostProcess>();
    postProcess_->Initialize();

    SceneManager::GetInstance()->SetPostProcess(postProcess_.get());
}

void App::Run() {
    // 現在のウィンドウの実際のクライアント領域サイズを取得し、サイズ変更を検知
    HWND hwnd = engine_->GetWindow()->GetHWND();
    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);
    int32_t currentWidth = clientRect.right - clientRect.left;
    int32_t currentHeight = clientRect.bottom - clientRect.top;

    static int32_t lastWidth = currentWidth;
    static int32_t lastHeight = currentHeight;

    if (currentWidth > 0 && currentHeight > 0 && 
        (currentWidth != lastWidth || currentHeight != lastHeight)) {
        
        // 1. スワップチェーンと深度バッファのリサイズ
        engine_->GetDxCommon()->ResizeSwapChain(currentWidth, currentHeight);

        // 2. ポストプロセスレンダーテクスチャのリサイズ
        postProcess_->Resize(currentWidth, currentHeight);

        // 3. カメラのプロジェクションアスペクト比の動的更新
        float aspect = static_cast<float>(currentWidth) / static_cast<float>(currentHeight);
        cameraMgr_->UpdateAllProjection(aspect);

        lastWidth = currentWidth;
        lastHeight = currentHeight;
    }

    bool isGameViewVisible = false;

    // --- ImGui ---
#ifdef _USEIMGUI
    engine_->ImGuiBegin();

    static bool showGameView = true;
    if (ImGui::BeginMainMenuBar()) {
        ImGui::Text("ZuizuiEngine");
        ImGui::Separator();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                PostQuitMessage(0); // 安全な終了メッセージを送信
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Game View", nullptr, &showGameView);
            ImGui::MenuItem("Console", nullptr, Log::GetShowConsolePtr());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            HWND hwnd = engine_->GetWindow()->GetHWND();
            
            static bool isFullscreen = false;
            if (ImGui::MenuItem("Fullscreen", "F11", &isFullscreen)) {
                static WINDOWPLACEMENT wpPrev = { sizeof(wpPrev) };
                DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
                
                // 元のサイズ変更不可のウィンドウスタイルをローカル定数定義（マジックナンバー排除）
                const DWORD kOriginalStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

                if (isFullscreen) {
                    // フルスクリーン化
                    MONITORINFO mi = { sizeof(mi) };
                    if (GetWindowPlacement(hwnd, &wpPrev) &&
                        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
                        SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                        SetWindowPos(hwnd, HWND_TOP,
                                     mi.rcMonitor.left, mi.rcMonitor.top,
                                     mi.rcMonitor.right - mi.rcMonitor.left,
                                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                    }
                } else {
                    // 元の画面サイズに戻す（明示的に 1280*720 とする）
                    SetWindowLong(hwnd, GWL_STYLE, kOriginalStyle);
                    
                    wpPrev.showCmd = SW_SHOWNORMAL;
                    // クライアント領域が 1280x720 になるように枠サイズを含めて計算
                    constexpr LONG kDefaultWidth = 1280;
                    constexpr LONG kDefaultHeight = 720;
                    RECT wr = { 0, 0, kDefaultWidth, kDefaultHeight };
                    AdjustWindowRect(&wr, kOriginalStyle, FALSE);
                    
                    wpPrev.rcNormalPosition.right = wpPrev.rcNormalPosition.left + (wr.right - wr.left);
                    wpPrev.rcNormalPosition.bottom = wpPrev.rcNormalPosition.top + (wr.bottom - wr.top);
                    
                    SetWindowPlacement(hwnd, &wpPrev);
                    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                }
            }
            
            ImGui::Separator();

            // アスペクト比変更メニューの追加
            static AspectType currentAspect = AspectType::Aspect16_9_Low;
            if (!isFullscreen && ImGui::BeginMenu("Aspect Ratio")) {
                const DWORD kOriginalStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
                
                auto ChangeWindowSize = [&](int32_t width, int32_t height) {
                    RECT wr = { 0, 0, width, height };
                    AdjustWindowRect(&wr, kOriginalStyle, FALSE);
                    
                    RECT rect{};
                    GetWindowRect(hwnd, &rect);
                    
                    SetWindowPos(hwnd, NULL,
                                 rect.left, rect.top,
                                 wr.right - wr.left,
                                 wr.bottom - wr.top,
                                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                };

                if (ImGui::MenuItem("16:9 (1280x720)", nullptr, currentAspect == AspectType::Aspect16_9_Low)) {
                    currentAspect = AspectType::Aspect16_9_Low;
                    ChangeWindowSize(1280, 720);
                }
                if (ImGui::MenuItem("16:9 (1920x1080)", nullptr, currentAspect == AspectType::Aspect16_9_High)) {
                    currentAspect = AspectType::Aspect16_9_High;
                    ChangeWindowSize(1920, 1080);
                }
                if (ImGui::MenuItem("4:3 (960x720)", nullptr, currentAspect == AspectType::Aspect4_3)) {
                    currentAspect = AspectType::Aspect4_3;
                    ChangeWindowSize(960, 720);
                }
                if (ImGui::MenuItem("1:1 (720x720)", nullptr, currentAspect == AspectType::Aspect1_1)) {
                    currentAspect = AspectType::Aspect1_1;
                    ChangeWindowSize(720, 720);
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Maximize")) {
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
            if (ImGui::MenuItem("Minimize")) {
                ShowWindow(hwnd, SW_MINIMIZE);
            }
            if (ImGui::MenuItem("Restore")) {
                ShowWindow(hwnd, SW_RESTORE);
            }
            
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // Game View ウィンドウ (UnityのGameビューのようにImGui内にゲーム画面を描画)
    if (showGameView) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin("Game View", &showGameView)) {
            isGameViewVisible = true;
            
            // ウィンドウサイズを取得し、アスペクト比を維持したサイズを計算
            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            
            // 現在のウィンドウの実際のクライアントアスペクト比を動的に計算して同期
            HWND hwnd = engine_->GetWindow()->GetHWND();
            RECT clientRect{};
            GetClientRect(hwnd, &clientRect);
            float screenWidth = static_cast<float>(clientRect.right - clientRect.left);
            float screenHeight = static_cast<float>(clientRect.bottom - clientRect.top);
            
            // ゼロ除算を防止する安全設計
            float currentAspectRatio = 16.0f / 9.0f;
            if (screenHeight > 0.0f) {
                currentAspectRatio = screenWidth / screenHeight;
            }
            
            float width = contentSize.x;
            float height = contentSize.x / currentAspectRatio;
            
            if (height > contentSize.y) {
                height = contentSize.y;
                width = contentSize.y * currentAspectRatio;
            }
            
            // 中央揃え用のパディング計算
            ImVec2 cursorPadding = ImVec2(
                (contentSize.x - width) * 0.5f,
                (contentSize.y - height) * 0.5f
            );
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + cursorPadding.x, ImGui::GetCursorPosY() + cursorPadding.y));
            
            // ポストプロセスの最終結果テクスチャを描画
            D3D12_GPU_DESCRIPTOR_HANDLE finalSrv = postProcess_->GetFinalSrvGpuHandle();
            ImTextureID texID = (ImTextureID)finalSrv.ptr;
            
            ImGui::Image(texID, ImVec2(width, height));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // Console ウィンドウの描画
    Log::DrawConsoleWindow();

    // 全シーン共通のデバッグメニュー
    ImGui::Begin("Scene Manager");
    ImGui::Text("Current Scene: %s", SceneManager::GetInstance()->GetCurrentSceneName().c_str());

    // マジックストリング回避のためのローカル定数定義
    static const std::string kDebugSceneName = "Debug";
    static const std::string kTitleSceneName = "Title";
    static const std::string kGameSceneName = "Game";
    static const std::string kClearSceneName = "Clear";
    static const std::string kGameOverSceneName = "GameOver";

    if (ImGui::Button("Reset DebugScene")) {
        SceneManager::GetInstance()->ChangeScene(kDebugSceneName);
    }
    if (ImGui::Button("Reset TitleScene")) {
        SceneManager::GetInstance()->ChangeScene(kTitleSceneName);
    }
    if (ImGui::Button("Reset GameScene")) {
        SceneManager::GetInstance()->ChangeScene(kGameSceneName);
    }
    if (ImGui::Button("Reset ClearScene")) {
        SceneManager::GetInstance()->ChangeScene(kClearSceneName);
    }
    if (ImGui::Button("Reset GameOverScene")) {
        SceneManager::GetInstance()->ChangeScene(kGameOverSceneName);
    }
    ImGui::End();

    SceneManager::GetInstance()->ImGuiControl();

    engine_->ImGuiEnd();
#endif

    // --- 更新 ---
    input_->Update();
    cameraMgr_->Update();
    lightMgr_->Update();

    SceneManager::GetInstance()->Update();

    // --- 描画 ---
    engine_->BeginFrame();

    // 1. ポストプロセス（RenderTexture）への描画パス
    postProcess_->PreDraw();

    SceneManager::GetInstance()->Draw();

    postProcess_->PostDraw();

    // 2. エフェクト適用およびスワップチェーンへのコピー処理
    if (isGameViewVisible) {
        // Game Viewが表示されている場合、エフェクト適用のみを行い、スワップチェーンへのコピーは不要
        postProcess_->ProcessEffects();
    } else {
        // Game Viewが表示されていない場合、通常どおりエフェクト適用＆スワップチェーン全体への描画コピーを行う
        postProcess_->Draw();
    }

    engine_->EndFrame();
}

void App::Finalize() {
    SceneManager::GetInstance()->ClearCurrentScene();
    EffectManager::GetInstance()->Finalize();
	engine_->Finalize();
}

bool App::IsEnd() const {
    return !engine_->ProcessMessage();
}
