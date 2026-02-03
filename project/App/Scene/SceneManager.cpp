#include "SceneManager.h"
#include <imGui.h>

SceneManager::SceneManager() {
    scene_ = SceneLabel::Title;
    titleScene_ = std::make_unique<TitleScene>();
    stageSelectScene_ = std::make_unique<StageSelectScene>();
    playScene_ = std::make_unique<PlayScene>();
    clearScene_ = std::make_unique<ClearScene>();
    gameOverScene_ = std::make_unique<GameOverScene>();
    currentScene_ = playScene_.get();
    audio_ = std::make_unique<Audio>();
}

SceneManager::~SceneManager() {}

void SceneManager::Initialize(SceneLabel scene, DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
    // 基盤システムのポインタを保存しておく
    dxCommon_ = dxCommon;
    psoManager_ = psoManager;
    textureManager_ = textureManager;
    input_ = input;

    audio_->Initialize();

    // 引数で初期化のシーンを選択
    switch (scene) {
    case SceneLabel::Title:
        currentScene_ = titleScene_.get();
        soundData_ = audio_->LoadSound("resources/AL/BGM/title.mp3");
        break;
    case SceneLabel::StageSelect:
        currentScene_ = stageSelectScene_.get();
        soundData_ = audio_->LoadSound("resources/AL/BGM/stageSelect.mp3");
        break;
    case SceneLabel::Play:
        currentScene_ = playScene_.get();
        soundData_ = audio_->LoadSound("resources/AL/BGM/game.mp3");
        break;
    case SceneLabel::Clear:
        currentScene_ = clearScene_.get();
        soundData_ = audio_->LoadSound("resources/AL/BGM/clear.mp3");
        break;
    case SceneLabel::Gameover:
        currentScene_ = gameOverScene_.get();
        soundData_ = audio_->LoadSound("resources/AL/BGM/gameOver.mp3");
        break;
    }

    // 現在のシーンに基盤を渡して初期化
    currentScene_->Initialize(dxCommon_, psoManager_, textureManager_, input_);
}

void SceneManager::Update() {
    currentScene_->Update();
    
    // シーン切り替えフラグが立っていた場合
    if (currentScene_->GetIsFinish()) {
        switch (currentScene_->GetNextScene()) {
        case SceneLabel::Title:
            currentScene_ = titleScene_.get();
            break;
        case SceneLabel::StageSelect:
            currentScene_ = stageSelectScene_.get();
            break;
        case SceneLabel::Play:
            currentScene_ = playScene_.get();
            break;
        case SceneLabel::Clear:
            currentScene_ = clearScene_.get();
            break;
        case SceneLabel::Gameover:
            currentScene_ = gameOverScene_.get();
            break;
        }

        // 次のシーンも同じ基盤システムで初期化する
        currentScene_->Initialize(dxCommon_, psoManager_, textureManager_, input_);
        audio_->StopSound(soundData_);
    }
}

void SceneManager::Draw() {
    currentScene_->Draw();
}

void SceneManager::ImGuiControl() {
    ImGui::Begin("Scene");
    switch (currentScene_->GetNowScene()) {
    case SceneLabel::Title:
        ImGui::Text("Scene = Title");
        break;
    case SceneLabel::StageSelect:
        ImGui::Text("Scene = StageSelect");
        break;
    case SceneLabel::Play:
        ImGui::Text("Scene = Play");
        break;
    case SceneLabel::Clear:
        ImGui::Text("Scene = Clear");
        break;
    case SceneLabel::Gameover:
        ImGui::Text("Scene = GameOver");
        break;
    }
    ImGui::End();
    currentScene_->ImGuiControl();
}
