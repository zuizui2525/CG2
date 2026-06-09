#include "App/App.h"
#include "App/Scene/Core/SceneManager.h"
#include "App/Scene/Core/SceneFactory.h"
#include "App/Load/ResourceLoader.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include <psapi.h> // メモリ取得用（追加）

#pragma comment(lib, "psapi.lib") // 追加

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
    static bool showPerfMonitor = true;
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
            ImGui::MenuItem("Performance Monitor", nullptr, &showPerfMonitor);
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

    // --- Performance Monitor ウィンドウの描画 ---
    if (showPerfMonitor) {
        if (ImGui::Begin("Performance Monitor", &showPerfMonitor)) {
            // 前回のフレームからの実際の経過時間を高精度測定（ジッターを拾うため）
            static auto lastFrameTime = std::chrono::steady_clock::now();
            auto currentFrameTime = std::chrono::steady_clock::now();
            float realDeltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
            lastFrameTime = currentFrameTime;

            // 極端な値を防ぐクランプ（最小: 10000 FPS, 最大: 1 FPS）
            if (realDeltaTime < 0.0001f) { realDeltaTime = 0.0001f; }
            if (realDeltaTime > 1.0f) { realDeltaTime = 1.0f; }

            float currentFps = 1.0f / realDeltaTime;
            float currentMs = realDeltaTime * 1000.0f;

            // プロセスの物理メモリ使用量（MB）を取得
            float currentMem = 0.0f;
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
                currentMem = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
            }

            // 履歴配列の定義（マジックナンバー排除用のconstexpr）
            constexpr int kHistorySize = 120;
            static float fpsHistory[kHistorySize] = {};
            static float midpointFpsHistory[kHistorySize] = {}; // 平均から中央値の履歴へ変更
            static float memHistory[kHistorySize] = {};
            static int historyOffset = 0;

            fpsHistory[historyOffset] = currentFps;
            memHistory[historyOffset] = currentMem;

            historyOffset = (historyOffset + 1) % kHistorySize;

            // 実行中の最高・最低FPSの記録（起動後60フレーム以降に記録開始）
            static float minObservedFps = -1.0f;
            static float maxObservedFps = -1.0f;
            static int frameCount = 0;
            frameCount++;
            if (frameCount > 60) {
                if (minObservedFps < 0.0f || currentFps < minObservedFps) {
                    minObservedFps = currentFps;
                }
                if (maxObservedFps < 0.0f || currentFps > maxObservedFps) {
                    maxObservedFps = currentFps;
                }
            }

            // 最高FPSと最低FPSの中央値を算出（黄色の線として描画）
            float midpointFps = currentFps;
            if (minObservedFps >= 0.0f && maxObservedFps >= 0.0f) {
                midpointFps = (maxObservedFps + minObservedFps) * 0.5f;
            }
            int lastWriteIdx = (historyOffset + kHistorySize - 1) % kHistorySize;
            midpointFpsHistory[lastWriteIdx] = midpointFps;

            // テキスト表示（最高・最低・中央値の数値をカラーで文字表記）
            ImGui::Text("FPS: %.1f", currentFps);
            if (minObservedFps >= 0.0f && maxObservedFps >= 0.0f) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), " (Min: %.1f)", minObservedFps);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), " Max: %.1f", maxObservedFps);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), " Mid: %.1f", midpointFps);
            }

            ImGui::Text("Latency: %.2f ms", currentMs);
            
            ImGui::Text("Memory: %.1f MB", currentMem);

            ImGui::Separator();
            
            // グラフの縦軸スケール上限を決定
            constexpr float kDefaultMaxFps = 75.0f;
            float maxFps = kDefaultMaxFps;
            for (int i = 0; i < kHistorySize; ++i) {
                if (fpsHistory[i] > maxFps) {
                    maxFps = fpsHistory[i];
                }
            }
            float graphMaxFps = maxFps;

            float maxMem = 128.0f;
            for (int i = 0; i < kHistorySize; ++i) {
                if (memHistory[i] > maxMem) {
                    maxMem = memHistory[i];
                }
            }
            float graphMaxMem = maxMem + 50.0f; // 50MBの余白

            // グラフ描画開始
            ImGui::Text("Performance Graph");

            // マージンとサイズの定義（マジックナンバー排除）
            constexpr float kLeftMargin = 50.0f;
            constexpr float kRightMargin = 80.0f;
            constexpr float kMinGraphHeight = 120.0f;
            constexpr float kBottomMargin = 20.0f;

            // グラフ全体の描画領域を確保するダミー要素を配置
            float totalWidth = ImGui::GetContentRegionAvail().x;
            float graphHeight = ImGui::GetContentRegionAvail().y - kBottomMargin;
            if (graphHeight < kMinGraphHeight) { graphHeight = kMinGraphHeight; }

            // ダミー要素を配置して領域を予約（アサーションエラー防止とレイアウト堅牢化）
            ImGui::Dummy(ImVec2(totalWidth, graphHeight));
            ImVec2 containerMin = ImGui::GetItemRectMin();
            ImVec2 containerMax = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // グラフ本体のサイズと描画位置を計算
            float graphWidth = totalWidth - (kLeftMargin + kRightMargin);
            if (graphWidth < 100.0f) { graphWidth = 100.0f; }
            ImVec2 kGraphSize = ImVec2(graphWidth, graphHeight);
            ImVec2 plotPos = ImVec2(containerMin.x + kLeftMargin, containerMin.y);

            // 1) 現在のFPS (緑) - 背景フレームあり
            ImGui::SetCursorScreenPos(plotPos);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));
            ImGui::PlotLines("##CurrentFPS", fpsHistory, kHistorySize, historyOffset, nullptr, 0.0f, graphMaxFps, kGraphSize);
            ImGui::PopStyleColor();

            // 2) 中央値FPS (黄) - 背景フレームを透明にして重ねて描画
            ImGui::SetCursorScreenPos(plotPos);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
            ImGui::PlotLines("##MidpointFPS", midpointFpsHistory, kHistorySize, historyOffset, nullptr, 0.0f, graphMaxFps, kGraphSize);
            ImGui::PopStyleColor(2);

            // 3) メモリ (水色) - 背景フレームを透明にして重ねて描画
            ImGui::SetCursorScreenPos(plotPos);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
            ImGui::PlotLines("##MemGraph", memHistory, kHistorySize, historyOffset, nullptr, 0.0f, graphMaxMem, kGraphSize);
            ImGui::PopStyleColor(2);

            // グラフ本体のホバー判定と矩形情報を取得
            bool isGraphHovered = ImGui::IsItemHovered();
            ImVec2 rectMin = ImGui::GetItemRectMin();
            ImVec2 rectMax = ImGui::GetItemRectMax();

            // グラフ値から画面上のY座標を逆算するヘルパーラムダ
            auto GetValueYPos = [&](float val, float scaleMax) -> float {
                float t = val / scaleMax;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                return rectMax.y - t * (rectMax.y - rectMin.y);
            };

            // 0FPS, 30FPS, 60FPS の左側目盛りテキストと水平補助線を描画
            const float fpsTickValues[] = { 0.0f, 30.0f, 60.0f };
            ImU32 tickTextColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
            ImU32 tickLineColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.08f)); // 極めて薄い白の補助線

            for (float val : fpsTickValues) {
                float yPos = GetValueYPos(val, graphMaxFps);
                
                // 目盛りテキストのフォーマット (例: "60FPS")
                char tickText[16];
                sprintf_s(tickText, "%.0fFPS", val);
                
                ImVec2 textSize = ImGui::CalcTextSize(tickText);
                // グラフの左端（rectMin.x）から少し左に寄せて、右揃えで配置する
                float textX = rectMin.x - textSize.x - 5.0f;
                // テキストの縦位置を線に合わせる
                float textY = yPos - textSize.y * 0.5f;

                // 左マージン内であれば描画する
                if (textX >= containerMin.x) {
                    drawList->AddText(ImVec2(textX, textY), tickTextColor, tickText);
                }

                // グラフ内に極めて薄い水平グリッド線を描画
                drawList->AddLine(ImVec2(rectMin.x, yPos), ImVec2(rectMax.x, yPos), tickLineColor, 1.0f);
            }

            // 4) 最高・最低FPSの補助線をグラフエリア内に描画
            if (minObservedFps >= 0.0f && maxObservedFps >= 0.0f) {
                float yMinLine = GetValueYPos(minObservedFps, graphMaxFps);
                float yMaxLine = GetValueYPos(maxObservedFps, graphMaxFps);

                // 薄い赤（最低）と薄い緑（最高）の横線を引く
                ImU32 minColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.3f, 0.3f, 0.35f));
                ImU32 maxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 1.0f, 0.3f, 0.35f));

                drawList->AddLine(ImVec2(rectMin.x, yMinLine), ImVec2(rectMax.x, yMinLine), minColor, 1.0f);
                drawList->AddLine(ImVec2(rectMin.x, yMaxLine), ImVec2(rectMax.x, yMaxLine), maxColor, 1.0f);

                // 線のすぐ上に識別テキストを小さく描画
                drawList->AddText(ImVec2(rectMin.x + 5.0f, yMinLine - 12.0f), minColor, "Min FPS");
                drawList->AddText(ImVec2(rectMin.x + 5.0f, yMaxLine - 12.0f), maxColor, "Max FPS");
            }

            // 5) 折れ線の右終端（最新値）のすぐ右側に識別名を描画（色はそれぞれの線と同色）
            // クリッピングを回避するため、Dummyの右端スペース（rectMax.x）の範囲内に描画する
            float yCurrent = GetValueYPos(currentFps, graphMaxFps);
            float yMidpoint = GetValueYPos(midpointFps, graphMaxFps);
            float yMemory = GetValueYPos(currentMem, graphMaxMem);

            // 近接時の衝突回避
            if (std::abs(yCurrent - yMidpoint) < 12.0f) {
                if (yCurrent < yMidpoint) {
                    yCurrent -= 6.0f;
                    yMidpoint += 6.0f;
                } else {
                    yCurrent += 6.0f;
                    yMidpoint -= 6.0f;
                }
            }

            ImU32 colorCurrent = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.2f, 1.0f)); // 緑
            ImU32 colorMidpoint = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.8f, 0.0f, 1.0f)); // 黄
            ImU32 colorMemory  = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.6f, 1.0f, 1.0f)); // 水色

            float labelX = rectMax.x + 5.0f;
            drawList->AddText(ImVec2(labelX, yCurrent - 6.0f), colorCurrent, "Current");
            drawList->AddText(ImVec2(labelX, yMidpoint - 6.0f), colorMidpoint, "Midpoint");
            drawList->AddText(ImVec2(labelX, yMemory - 6.0f), colorMemory, "Memory");

            // マウスホバー時にカーソル位置に対応する詳細データ（過去データ）をツールチップ表示
            if (isGraphHovered) {
                ImVec2 mousePos = ImGui::GetMousePos();

                float fraction = (mousePos.x - rectMin.x) / (rectMax.x - rectMin.x);
                int hoveredDataIdx = static_cast<int>(fraction * kHistorySize);
                if (hoveredDataIdx < 0) { hoveredDataIdx = 0; }
                if (hoveredDataIdx >= kHistorySize) { hoveredDataIdx = kHistorySize - 1; }

                int actualIdx = (historyOffset + hoveredDataIdx) % kHistorySize;

                float hoverFps = fpsHistory[actualIdx];
                float hoverMidFps = midpointFpsHistory[actualIdx];
                float hoverMem = memHistory[actualIdx];

                ImGui::BeginTooltip();
                int framesAgo = kHistorySize - 1 - hoveredDataIdx;
                ImGui::Text("Time: %d frames ago (approx. %.1fs ago)", framesAgo, static_cast<float>(framesAgo) * 0.016f);
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.2f, 1.0f), "Current: %.1f FPS", hoverFps);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Midpoint: %.1f FPS", hoverMidFps);
                ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Memory: %.1f MB", hoverMem);
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
    }

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
