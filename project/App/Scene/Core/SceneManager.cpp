#include "App/Scene/Core/SceneManager.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/StringUtility.h"
#include <format>

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

        // 開始前に空行を挿入して可読性を向上
        Log::Write(L"");

        Log::Write(L"========================================= [シーン切り替え開始] =========================================");
        Log::Write(std::format(L" ├─ 【シーン遷移開始】 新しいシーン「{}」への遷移を開始します。古いシーン「{}」を破棄します。", ConvertString(nextSceneName_), ConvertString(currentSceneName_)));

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

            Log::Write(std::format(L" ├─ 【シーン遷移完了】 「{}」シーンへ遷移し、初期化しました。", ConvertString(currentSceneName_)));
            Log::Write(L"========================================= [シーン切り替え完了] =========================================");

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
