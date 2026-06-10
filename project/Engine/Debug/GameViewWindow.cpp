#ifdef _USEIMGUI
#include "Engine/Debug/GameViewWindow.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/PostProcess/PostProcess.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Debug/ReplaySystem.h"
#include "externals/imgui/imgui.h"
#include <windows.h>

GameViewWindow::GameViewWindow()
    : wasPaused_(false) {
}

void GameViewWindow::Draw(bool* show, bool* isVisible) {
    *isVisible = false;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Game View", show)) {
        *isVisible = true;

        PostProcess* postProcess = SceneManager::GetInstance()->GetPostProcess();
        if (postProcess) {
            // ウィンドウサイズを取得し、アスペクト比を維持したサイズを計算
            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            
            // 現在のウィンドウの実際のクライアントアスペクト比を動的に計算して同期
            HWND hwnd = Zuizui::GetInstance()->GetWindow()->GetHWND();
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
            D3D12_GPU_DESCRIPTOR_HANDLE finalSrv = postProcess->GetFinalSrvGpuHandle();
            ImTextureID texID = (ImTextureID)finalSrv.ptr;
            
            ImVec2 imgPosMin = ImGui::GetCursorScreenPos();
            ImGui::Image(texID, ImVec2(width, height));

            // 中央座標の計算
            ImVec2 center = ImVec2(imgPosMin.x + width * 0.5f, imgPosMin.y + height * 0.5f);

            // ポーズ状態の監視と演出トリガー
            bool currentPaused = ReplaySystem::GetInstance()->IsPaused();
            if (currentPaused != wasPaused_) {
                if (ReplaySystem::GetInstance()->GetRecordCount() > 0) {
                    popAnim_.Trigger(currentPaused ? PopAnimation::Type::Pause : PopAnimation::Type::Play);
                }
                wasPaused_ = currentPaused;
            }

            // アニメーションの更新と描画
            popAnim_.Update(ImGui::GetIO().DeltaTime);
            popAnim_.Draw(ImGui::GetWindowDrawList(), center);

            // 画像領域のタップ（左クリック）でゲームの再生/一時停止をトグル
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                bool isPaused = ReplaySystem::GetInstance()->IsPaused();
                ReplaySystem::GetInstance()->SetPause(!isPaused);
            }

            // 一時停止中はYouTube風のポーズ画面（半透明グレーアウト）を表示
            if (ReplaySystem::GetInstance()->IsPaused()) {
                ImVec2 rectMin = ImGui::GetItemRectMin();
                ImVec2 rectMax = ImGui::GetItemRectMax();

                // 1. 半透明グレーのオーバーレイ（透明度を下げて視認性を向上：120 ➡ 80）
                constexpr ImU32 kOverlayColor = IM_COL32(20, 20, 20, 80);
                ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, kOverlayColor);
            }
        } else {
            ImGui::Text("No Active PostProcess");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
#endif
