#ifdef _USEIMGUI
#include "Engine/Debug/DebugEditor.h"
#include "Engine/Debug/GameViewWindow.h"
#include "Engine/Debug/PerformanceMonitorWindow.h"
#include "Engine/Debug/SceneManagerWindow.h"
#include "Engine/Debug/SceneHierarchy.h"
#include "Engine/Debug/IGameObject.h"
#include "Engine/Zuizui.h"
#include "App/Scene/Core/SceneManager.h"
#include "App/Scene/Game/GameScene.h"
#include "Engine/Base/Log/Log.h"
#include "externals/imgui/imgui.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"

DebugEditor::DebugEditor()
    : showGameView_(true),
      showPerfMonitor_(true),
      isGameViewVisible_(false),
      isFullscreen_(false),
      currentAspect_(AspectType::Aspect16_9_Low),
      isPaused_(false) {
    wpPrev_.length = sizeof(wpPrev_);
}

DebugEditor::~DebugEditor() = default;

void DebugEditor::Initialize() {
    gameViewWindow_ = std::make_unique<GameViewWindow>();
    perfMonitorWindow_ = std::make_unique<PerformanceMonitorWindow>();
    sceneManagerWindow_ = std::make_unique<SceneManagerWindow>();
}

void DebugEditor::Draw(ID3D12GraphicsCommandList* commandList) {
    // 描画開始時に可視性フラグを初期化
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

    // Console
    Log::DrawConsoleWindow(-1.0f);

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
            ImGui::Selectable(obj->GetName().c_str(), isSelected);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                SceneHierarchy::GetInstance()->SetSelected(obj);
                gameViewWindow_->SetGizmoOperation(7); // TRANSLATE
            } else if (ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
                SceneHierarchy::GetInstance()->SetSelected(obj);
                gameViewWindow_->SetGizmoOperation(120); // ROTATE
            } else if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                SceneHierarchy::GetInstance()->SetSelected(obj);
                gameViewWindow_->SetGizmoOperation(896); // SCALE
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
            
            GameScene* gameScene = dynamic_cast<GameScene*>(SceneManager::GetInstance()->GetCurrentScene());
            if (gameScene) {
                ImGui::Separator();
                ImGui::MenuItem("Stage Editor", nullptr, gameScene->GetShowStageEditorPtr());
                ImGui::MenuItem("Route Editor", nullptr, gameScene->GetShowRouteEditorPtr());
            }

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

        // 画面中央付近に再生/一時停止ボタンを配置
        float menuBarWidth = ImGui::GetWindowWidth();
        constexpr float kPlayPauseBtnWidth = 70.0f;
        float centerPos = (menuBarWidth - kPlayPauseBtnWidth) * 0.5f;
        ImGui::SameLine(centerPos);

        if (isPaused_) {
            if (ImGui::Button("Play ▶")) {
                isPaused_ = false;
            }
        } else {
            if (ImGui::Button("Pause ||")) {
                isPaused_ = true;
            }
        }

        if (gameViewWindow_) {
            ImGui::SameLine();
            bool enable = gameViewWindow_->IsClickPauseEnabled();
            if (ImGui::Checkbox("Click Pause", &enable)) {
                gameViewWindow_->SetClickPauseEnabled(enable);
            }

            ImGui::SameLine();
            bool showGizmo = gameViewWindow_->IsShowGizmo();
            if (ImGui::Checkbox("Use Gizmo", &showGizmo)) {
                gameViewWindow_->SetShowGizmo(showGizmo);
            }
        }

        ImGui::EndMainMenuBar();
    }
}
#endif
