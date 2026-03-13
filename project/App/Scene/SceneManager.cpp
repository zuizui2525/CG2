#include "SceneManager.h"
#include "BaseResource.h"
#include "CameraManager.h"
#include "LightManager.h"

SceneManager* SceneManager::GetInstance() {
    static SceneManager instance;
    return &instance;
}

void SceneManager::ImGuiControl() {
    if (currentScene_) {
        currentScene_->ImGuiControl();
    }
}

void SceneManager::Update() {
    // 次のシーンが予約されていたら切り替える
    if (nextScene_) {
        // ライトとカメラのリセット
        CameraResource::GetCameraManager()->Clear();
        LightResource::GetLightManager()->Clear();
        // currentScene_に代入することで、古いシーンは自動的に破棄される
        currentScene_ = std::move(nextScene_);
        currentSceneName_ = nextSceneName_;

        // 新しいシーンの初期化
        currentScene_->Initialize();

        nextScene_ = nullptr;
    }

    // 現在のシーンがあれば更新を実行
    if (currentScene_) {
        currentScene_->Update();
    }
}

void SceneManager::Draw() {
    // 現在のシーンがあれば描画を実行
    if (currentScene_) {
        currentScene_->Draw();
    }
}
