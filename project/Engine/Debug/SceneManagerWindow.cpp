#ifdef _USEIMGUI
#include "Engine/Debug/SceneManagerWindow.h"
#include "App/Scene/Core/SceneManager.h"
#include "externals/imgui/imgui.h"
#include <string>

void SceneManagerWindow::Draw() {
    // 全シーン共通のデバッグメニュー
    if (ImGui::Begin("Scene Manager")) {
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
    }
    ImGui::End();
}
#endif
