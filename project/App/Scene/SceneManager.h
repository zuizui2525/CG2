#pragma once
#include "Scene.h"
#include "TitleScene.h"
#include "StageSelectScene.h"
#include "PlayScene.h"
#include "ClearScene.h"
#include "GameOverScene.h"
#include "Audio.h"
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
    // BGMの切り替え処理（内部用）
    void ChangeBGM(SceneLabel scene);

private:
    std::unique_ptr<TitleScene> titleScene_;
    std::unique_ptr<StageSelectScene> stageSelectScene_;
    std::unique_ptr<PlayScene> playScene_;
    std::unique_ptr<ClearScene> clearScene_;
    std::unique_ptr<GameOverScene> gameOverScene_;

    SceneLabel scene_;
    Scene* currentScene_;

    std::unique_ptr<Audio> audio_;
    SoundData soundData_;

    // 基盤システムのポインタ保持用
    DxCommon* dxCommon_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    Input* input_ = nullptr;
};
