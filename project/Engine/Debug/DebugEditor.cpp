#ifdef _USEIMGUI
#include "Engine/Debug/DebugEditor.h"
#include "Engine/Debug/GameViewWindow.h"
#include "Engine/Debug/PerformanceMonitorWindow.h"
#include "Engine/Debug/SceneManagerWindow.h"
#include "Engine/Debug/ReplaySystem.h"
#include "Engine/Debug/SceneHierarchy.h"
#include "Engine/Debug/IGameObject.h"
#include "Engine/Zuizui.h"
#include "Engine/Base/Log/Log.h"
#include "externals/imgui/imgui.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"

DebugEditor::DebugEditor()
    : showGameView_(true),
      showPerfMonitor_(true),
      showReplayView_(true),
      isGameViewVisible_(false),
      isFullscreen_(false),
      currentAspect_(AspectType::Aspect16_9_Low),
      wasReplayPlaying_(false) {
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

    // Hierarchy (左側)
    if (ImGui::Begin("Hierarchy")) {
        const auto& objects = SceneHierarchy::GetInstance()->GetObjects();
        IGameObject* selected = SceneHierarchy::GetInstance()->GetSelected();

        for (auto* obj : objects) {
            // 表示フラグ用のチェックボックス
            bool isVisible = obj->IsVisible();
            std::string chkLabel = "##visible_" + obj->GetName();
            if (ImGui::Checkbox(chkLabel.c_str(), &isVisible)) {
                obj->SetVisible(isVisible);
            }
            ImGui::SameLine();

            // 選択状態
            bool isSelected = (obj == selected);
            if (ImGui::Selectable(obj->GetName().c_str(), isSelected)) {
                SceneHierarchy::GetInstance()->SetSelected(obj);
            }
        }
    }
    ImGui::End();

    // Inspector (右側)
    if (ImGui::Begin("Inspector")) {
        IGameObject* selected = SceneHierarchy::GetInstance()->GetSelected();
        if (selected) {
            // 名前の編集
            constexpr int kNameBufferSize = 128;
            char nameBuf[kNameBufferSize];
            strcpy_s(nameBuf, selected->GetName().c_str());
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                selected->SetName(nameBuf);
            }

            ImGui::Separator();

            // 各種オブジェクト固有のインスペクター描画
            selected->DrawInspector();
        } else {
            ImGui::Text("No object selected.");
        }
    }
    ImGui::End();

    // Replay View (表示フラグ showReplayView_ に連動)
    if (showReplayView_) {
        if (ImGui::Begin("Replay View", &showReplayView_)) {
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
            
            int32_t startIdx = 0;
            int32_t activeCount = ReplaySystem::GetInstance()->GetEffectiveRecordCount(&startIdx);

            float secondsAgo = 0.0f;
            if (activeCount > 0) {
                // 現在のシーク位置に対応するインデックスを計算
                int32_t targetIdx = startIdx + static_cast<int32_t>(progress * (activeCount - 1));
                targetIdx = std::clamp(targetIdx, startIdx, startIdx + activeCount - 1);
                
                // 実際のタイムスタンプ差分から、正確な経過秒数を取得（FPSハードコードを完全排除）
                secondsAgo = ReplaySystem::GetInstance()->GetReplayTimeOffset(targetIdx);
            }

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
                    if (ImGui::Button("Play ▶")) {
                        // シークバーが最後まで達している場合は、最初から再生するために 0 に戻す
                        if (progress >= 1.0f) {
                            ReplaySystem::GetInstance()->SetSeekPos(0.0f);
                        }
                        ReplaySystem::GetInstance()->SetReplayPlaying(true);
                    }
                }
                ImGui::SameLine();

                // 再生速度選択UI (Combo)
                float currentSpeed = ReplaySystem::GetInstance()->GetPlaySpeed();
                const char* previewLabel = "1.0x";
                
                constexpr float kSpeeds[] = { 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 4.0f, 8.0f };
                constexpr const char* kSpeedLabels[] = { "0.25x", "0.5x", "1.0x", "1.5x", "2.0x", "4.0x", "8.0x" };
                constexpr int32_t kSpeedCount = static_cast<int32_t>(std::size(kSpeeds));

                for (int32_t i = 0; i < kSpeedCount; ++i) {
                    if (std::abs(currentSpeed - kSpeeds[i]) < 0.01f) {
                        previewLabel = kSpeedLabels[i];
                        break;
                    }
                }

                // コンボボックスの横幅を固定 (80ピクセル)
                ImGui::SetNextItemWidth(80.0f);
                if (ImGui::BeginCombo("##Speed", previewLabel)) {
                    for (int32_t i = 0; i < kSpeedCount; ++i) {
                        const bool isSelected = (std::abs(currentSpeed - kSpeeds[i]) < 0.01f);
                        if (ImGui::Selectable(kSpeedLabels[i], isSelected)) {
                            ReplaySystem::GetInstance()->SetPlaySpeed(kSpeeds[i]);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
            }

            // シークバーの描画 (残りの横幅いっぱいにフィットさせる)
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##Seek", &progress, 0.0f, 1.0f, sliderLabel)) {
                ReplaySystem::GetInstance()->SetSeekPos(progress);
            }

            // 画像のタップクリック判定を有効化するため、ここで操作制限（Disabled）を解除
            if (!isPaused) {
                ImGui::EndDisabled();
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

                ImVec2 imgPosMin = ImGui::GetCursorScreenPos();
                ImGui::Image((ImTextureID)srvGpu.ptr, ImVec2(drawWidth, drawHeight));

                // 中央座標の計算
                ImVec2 center = ImVec2(imgPosMin.x + drawWidth * 0.5f, imgPosMin.y + drawHeight * 0.5f);

                // リプレイ再生状態の監視とトリガー
                bool currentReplayPlaying = ReplaySystem::GetInstance()->IsReplayPlaying();
                if (currentReplayPlaying != wasReplayPlaying_) {
                    if (isPaused) {
                        replayPopAnim_.Trigger(currentReplayPlaying ? PopAnimation::Type::Play : PopAnimation::Type::Pause);
                    }
                    wasReplayPlaying_ = currentReplayPlaying;
                }

                // アニメーションの更新と描画
                replayPopAnim_.Update(ImGui::GetIO().DeltaTime);
                replayPopAnim_.Draw(ImGui::GetWindowDrawList(), center);

                // 画像領域のタップ（左クリック）で再生/一時停止をトグル
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (isPaused) {
                        // 一時停止中はリプレイの自動再生/一時停止をトグル
                        bool isReplayPlaying = ReplaySystem::GetInstance()->IsReplayPlaying();
                        if (!isReplayPlaying) {
                            // シーク位置が最後まで達している場合は、最初から再生するため0に戻す
                            if (progress >= 1.0f) {
                                ReplaySystem::GetInstance()->SetSeekPos(0.0f);
                            }
                        }
                        ReplaySystem::GetInstance()->SetReplayPlaying(!isReplayPlaying);
                    } else {
                        // 通常動作中はゲームを一時停止にする
                        ReplaySystem::GetInstance()->SetPause(true);
                    }
                }

                // ゲーム実行中（未ポーズ）で、かつリプレイ映像が静止している場合のみ、中央に▶マークを常時表示（クリックによる自動一時停止ガイド）
                if (!ReplaySystem::GetInstance()->IsReplayPlaying() && !isPaused) {
                    ImVec2 rectMin = ImGui::GetItemRectMin();
                    ImVec2 rectMax = ImGui::GetItemRectMax();

                    // 1. 半透明グレーのオーバーレイ（透明度を下げて視認性を向上：120 ➡ 80）
                    constexpr ImU32 kOverlayColor = IM_COL32(20, 20, 20, 80);
                    ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, kOverlayColor);

                    // 中央座標
                    ImVec2 center = ImVec2((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);

                    // 2. YouTube風の円形背景（透明度を下げてゲーム画面に馴染むように：160 ➡ 110）
                    constexpr float kCircleRadius = 40.0f;
                    constexpr ImU32 kCircleColor = IM_COL32(0, 0, 0, 110);
                    constexpr int32_t kCircleSegments = 36;
                    ImGui::GetWindowDrawList()->AddCircleFilled(center, kCircleRadius, kCircleColor, kCircleSegments);

                    // 3. 白い再生三角形（▶）の描画（透明度を下げて落ち着いた半透明白に：240 ➡ 170）
                    // 視覚的重心ズレ（右向き三角形特有の右寄りの偏り）を補正するため、X軸補正を -1.5f に変更し完全なセンタリングを行います。
                    constexpr float kTriangleSize = 30.0f;
                    constexpr float kH = kTriangleSize * 0.866f;
                    constexpr float kXOffset = -1.5f; 
                    constexpr float kYOffset = -1.0f;
                    ImVec2 p1(center.x - kH * 0.333f + kXOffset, center.y - kTriangleSize * 0.5f + kYOffset);
                    ImVec2 p2(center.x - kH * 0.333f + kXOffset, center.y + kTriangleSize * 0.5f + kYOffset);
                    ImVec2 p3(center.x + kH * 0.667f + kXOffset, center.y + kYOffset);
                    constexpr ImU32 kTriangleColor = IM_COL32(255, 255, 255, 170);
                    ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, kTriangleColor);
                }
            } else {
                ImGui::Text("No replay data buffered yet.");
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
            ImGui::MenuItem("Replay View", nullptr, &showReplayView_);
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
            if (ImGui::Button("Play ▶")) {
                ReplaySystem::GetInstance()->SetPause(false);
            }
        } else {
            if (ImGui::Button("Pause ||")) {
                ReplaySystem::GetInstance()->SetPause(true);
            }
        }

        // Playボタンの右側に左クリック一時停止有効・無効化のチェックボックスを表示
        if (gameViewWindow_) {
            ImGui::SameLine();
            bool enable = gameViewWindow_->IsClickPauseEnabled();
            if (ImGui::Checkbox("Click Pause", &enable)) {
                gameViewWindow_->SetClickPauseEnabled(enable);
            }
        }

        ImGui::EndMainMenuBar();
    }
}
#endif
