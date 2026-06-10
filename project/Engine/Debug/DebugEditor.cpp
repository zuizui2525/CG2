#ifdef _USEIMGUI
#include "Engine/Debug/DebugEditor.h"
#include "Engine/Debug/GameViewWindow.h"
#include "Engine/Debug/PerformanceMonitorWindow.h"
#include "Engine/Debug/SceneManagerWindow.h"
#include "Engine/Debug/ReplaySystem.h"
#include "Engine/Zuizui.h"
#include "Engine/Base/Log/Log.h"
#include "externals/imgui/imgui.h"

DebugEditor::DebugEditor()
    : showGameView_(true),
      showPerfMonitor_(true),
      isGameViewVisible_(false),
      isFullscreen_(false),
      currentAspect_(AspectType::Aspect16_9_Low) {
    wpPrev_.length = sizeof(wpPrev_);
}

DebugEditor::~DebugEditor() = default;

void DebugEditor::Initialize() {
    gameViewWindow_ = std::make_unique<GameViewWindow>();
    perfMonitorWindow_ = std::make_unique<PerformanceMonitorWindow>();
    sceneManagerWindow_ = std::make_unique<SceneManagerWindow>();
}

void DebugEditor::Draw(ID3D12GraphicsCommandList* commandList) {
    // リプレイ自動送り再生の更新
    ReplaySystem::GetInstance()->UpdateReplayPlay(ImGui::GetIO().DeltaTime);

    HWND hwnd = Zuizui::GetInstance()->GetWindow()->GetHWND();

    // メインメニューバーの描画
    DrawMenuBar(hwnd);

    // ドックスペースの設定
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // Game View
    if (showGameView_) {
        gameViewWindow_->Draw(&showGameView_, &isGameViewVisible_);
    } else {
        isGameViewVisible_ = false;
    }

    // Console (リプレイ中はタイムスタンプの上限を渡す)
    float maxTimestamp = ReplaySystem::GetInstance()->GetReplayMaxTimestamp();
    Log::DrawConsoleWindow(maxTimestamp);

    // Performance Monitor
    if (showPerfMonitor_) {
        perfMonitorWindow_->Draw(&showPerfMonitor_);
    }

    // Scene Manager
    sceneManagerWindow_->Draw();

    // Replay View (常時表示)
    if (true) {
        if (ImGui::Begin("Replay View")) {
            bool isPaused = ReplaySystem::GetInstance()->IsPaused();

            if (!isPaused) {
                // 通常再生中は操作不可として警告を表示し、全体をグレーアウト
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Replay is only available while PAUSED.");
                ImGui::Text("Click 'Pause ||' at the top main menu bar.");
                ImGui::Separator();
            }

            if (!isPaused) {
                ImGui::BeginDisabled();
            }

            int32_t recordCount = ReplaySystem::GetInstance()->GetRecordCount();
            float progress = ReplaySystem::GetInstance()->GetSeekPos();
            
            // 記録フレームに対応する秒数を計算 (5fps想定)
            float maxSeconds = static_cast<float>(recordCount) / 5.0f;
            float secondsAgo = (1.0f - progress) * maxSeconds;

            char sliderLabel[64];
            if (recordCount == 0) {
                sprintf_s(sliderLabel, "No Replay Data");
            } else if (secondsAgo <= 0.0f) {
                sprintf_s(sliderLabel, "Current (0.0s ago)");
            } else {
                sprintf_s(sliderLabel, "-%.1fs ago", secondsAgo);
            }

            // シークバーの左に Play / Pause ボタンを配置
            if (recordCount > 0 && isPaused) {
                bool isReplayPlaying = ReplaySystem::GetInstance()->IsReplayPlaying();
                if (isReplayPlaying) {
                    if (ImGui::Button("Pause ||")) {
                        ReplaySystem::GetInstance()->SetReplayPlaying(false);
                    }
                } else {
                    if (ImGui::Button("Play  >")) {
                        // シークバーが最後まで達している場合は、最初から再生するために 0 に戻す
                        if (progress >= 1.0f) {
                            ReplaySystem::GetInstance()->SetSeekPos(0.0f);
                        }
                        ReplaySystem::GetInstance()->SetReplayPlaying(true);
                    }
                }
                ImGui::SameLine();
            }

            // シークバーの描画 (残りの横幅いっぱいにフィットさせる)
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##Seek", &progress, 0.0f, 1.0f, sliderLabel)) {
                ReplaySystem::GetInstance()->SetSeekPos(progress);
            }

            ImGui::Separator();

            // リプレイ画像
            if (recordCount > 0) {
                D3D12_GPU_DESCRIPTOR_HANDLE srvGpu = ReplaySystem::GetInstance()->GetReplaySrvGpuHandle();
                
                // アスペクト比を保って描画領域にフィットさせる (デフォルト16:9)
                float contentWidth = ImGui::GetContentRegionAvail().x;
                float contentHeight = ImGui::GetContentRegionAvail().y;
                
                constexpr float kDefaultAspect = 16.0f / 9.0f;
                float drawWidth = contentWidth;
                float drawHeight = contentWidth / kDefaultAspect;

                if (drawHeight > contentHeight) {
                    drawHeight = contentHeight;
                    drawWidth = contentHeight * kDefaultAspect;
                }

                ImGui::Image((ImTextureID)srvGpu.ptr, ImVec2(drawWidth, drawHeight));
            } else {
                ImGui::Text("No replay data buffered yet.");
            }

            if (!isPaused) {
                ImGui::EndDisabled();
            }
        }
        ImGui::End();
    }
}

void DebugEditor::DrawMenuBar(HWND hwnd) {
    if (ImGui::BeginMainMenuBar()) {
        ImGui::Text("ZuizuiEngine");
        ImGui::Separator();
        
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Game View", nullptr, &showGameView_);
            ImGui::MenuItem("Console", nullptr, Log::GetShowConsolePtr());
            ImGui::MenuItem("Performance Monitor", nullptr, &showPerfMonitor_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Fullscreen", "F11", &isFullscreen_)) {
                DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
                
                // 元のサイズ変更不可のウィンドウスタイルをローカル定数定義（マジックナンバー排除）
                const DWORD kOriginalStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

                if (isFullscreen_) {
                    // フルスクリーン化
                    MONITORINFO mi = { sizeof(mi) };
                    if (GetWindowPlacement(hwnd, &wpPrev_) &&
                        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
                        SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                        SetWindowPos(hwnd, HWND_TOP,
                                     mi.rcMonitor.left, mi.rcMonitor.top,
                                     mi.rcMonitor.right - mi.rcMonitor.left,
                                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                    }
                } else {
                    // 元の画面サイズに戻す
                    SetWindowLong(hwnd, GWL_STYLE, kOriginalStyle);
                    
                    wpPrev_.showCmd = SW_SHOWNORMAL;
                    
                    // 直前のアスペクト比設定に応じたクライアント解像度を決定（マジックナンバー排除）
                    int32_t restoreWidth = 1280;
                    int32_t restoreHeight = 720;
                    switch (currentAspect_) {
                    case AspectType::Aspect16_9_Low:
                        restoreWidth = 1280;
                        restoreHeight = 720;
                        break;
                    case AspectType::Aspect16_9_High:
                        restoreWidth = 1920;
                        restoreHeight = 1080;
                        break;
                    case AspectType::Aspect4_3:
                        restoreWidth = 960;
                        restoreHeight = 720;
                        break;
                    case AspectType::Aspect1_1:
                        restoreWidth = 720;
                        restoreHeight = 720;
                        break;
                    }

                    RECT wr = { 0, 0, restoreWidth, restoreHeight };
                    AdjustWindowRect(&wr, kOriginalStyle, FALSE);
                    
                    wpPrev_.rcNormalPosition.right = wpPrev_.rcNormalPosition.left + (wr.right - wr.left);
                    wpPrev_.rcNormalPosition.bottom = wpPrev_.rcNormalPosition.top + (wr.bottom - wr.top);
                    
                    SetWindowPlacement(hwnd, &wpPrev_);
                    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                }
            }
            
            ImGui::Separator();

            // アスペクト比変更メニュー
            if (!isFullscreen_ && ImGui::BeginMenu("Aspect Ratio")) {
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

                if (ImGui::MenuItem("16:9 (1280x720)", nullptr, currentAspect_ == AspectType::Aspect16_9_Low)) {
                    currentAspect_ = AspectType::Aspect16_9_Low;
                    ChangeWindowSize(1280, 720);
                }
                if (ImGui::MenuItem("16:9 (1920x1080)", nullptr, currentAspect_ == AspectType::Aspect16_9_High)) {
                    currentAspect_ = AspectType::Aspect16_9_High;
                    ChangeWindowSize(1920, 1080);
                }
                if (ImGui::MenuItem("4:3 (960x720)", nullptr, currentAspect_ == AspectType::Aspect4_3)) {
                    currentAspect_ = AspectType::Aspect4_3;
                    ChangeWindowSize(960, 720);
                }
                if (ImGui::MenuItem("1:1 (720x720)", nullptr, currentAspect_ == AspectType::Aspect1_1)) {
                    currentAspect_ = AspectType::Aspect1_1;
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

        // 画面中央付近に再生/一時停止ボタンを配置 (マジックナンバー排除)
        float menuBarWidth = ImGui::GetWindowWidth();
        constexpr float kPlayPauseBtnWidth = 70.0f;
        float centerPos = (menuBarWidth - kPlayPauseBtnWidth) * 0.5f;
        ImGui::SameLine(centerPos);

        if (ReplaySystem::GetInstance()->IsPaused()) {
            if (ImGui::Button("Play  >")) {
                ReplaySystem::GetInstance()->SetPause(false);
            }
        } else {
            if (ImGui::Button("Pause ||")) {
                ReplaySystem::GetInstance()->SetPause(true);
            }
        }

        ImGui::EndMainMenuBar();
    }
}
#endif
