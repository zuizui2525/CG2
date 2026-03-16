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
    // 次のシーン名が入っていたら切り替え処理を行う
    if (!nextSceneName_.empty()) {

        // 工場がセットされていない場合はエラー（または早期リターン）
        if (!sceneFactory_) return;

        // 【Factory Methodパターンの核心】
        // 名前（文字列）を工場に渡し、具体的なクラスを意識せずにインスタンスを得る
        nextScene_ = sceneFactory_->CreateScene(nextSceneName_);

        if (nextScene_) {
            // ライトとカメラのリセット
            CameraResource::GetCameraManager()->Clear();
            LightResource::GetLightManager()->Clear();

            // 古いシーンを破棄して新しいシーンへ
            currentScene_ = std::move(nextScene_);
            currentSceneName_ = nextSceneName_;

            // 新しいシーンの初期化
            currentScene_->Initialize();
        }

        // 予約名をクリア
        nextSceneName_.clear();
    }

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
