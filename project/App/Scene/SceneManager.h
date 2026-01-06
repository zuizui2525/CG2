#pragma once
#include "Scene.h"
#include "TitleScene.h"
#include "PlayScene.h"
#include <memory>

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Initialize(SceneLabel scene, DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input);
    void Update();
    void Draw();
    void ImGuiControl();

private:
    std::unique_ptr<TitleScene> titleScene_;
    std::unique_ptr<PlayScene> playScene_;

    SceneLabel scene_;
    Scene* currentScene_;

    // 後ほどシーン切り替えが発生した時のために、ポインタを保持しておく
    DxCommon* dxCommon_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    Input* input_ = nullptr;
};
