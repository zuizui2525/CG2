#pragma once
#include <memory>
#include <string>
#include "IScene.h"

class SceneManager {
public:
    // シングルトンインスタンスの取得
    static SceneManager* GetInstance();

    // Appクラスのメインループから呼ばれる
    void Update();
    void Draw();
    void ImGuiControl();

    // シーン切り替え予約
    // 次のフレームのUpdate冒頭で新しいシーンを生成する
    template <typename T>
    void ChangeScene(const std::string& sceneName) {
        nextScene_ = std::make_unique<T>();
        nextSceneName_ = sceneName;
    }

    void ClearCurrentScene() { currentScene_.reset(); }

    // デバッグ表示用に現在のシーン名を取得
    const std::string& GetCurrentSceneName() const { return currentSceneName_; }

private:
    SceneManager() = default;
    ~SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    std::unique_ptr<IScene> currentScene_ = nullptr; // 現在実行中のシーン
    std::unique_ptr<IScene> nextScene_ = nullptr;    // 切り替え予約された次のシーン
    std::string currentSceneName_ = "None";          // 現在のシーン名
    std::string nextSceneName_ = "";                 // 次のシーン名の予約
};
