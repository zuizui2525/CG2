#include "SceneManager.h"
#include <imGui.h>

SceneManager::SceneManager() {
    scene_ = SceneLabel::Title;
    titleScene_ = std::make_unique<TitleScene>();
    stageSelectScene_ = std::make_unique<StageSelectScene>();
    playScene_ = std::make_unique<PlayScene>();
    clearScene_ = std::make_unique<ClearScene>();
    gameOverScene_ = std::make_unique<GameOverScene>();
    currentScene_ = titleScene_.get(); // 初期シーンをTitleに設定
    audio_ = std::make_unique<Audio>();
}

SceneManager::~SceneManager() {
    audio_->StopSound(soundData_);
    audio_->Unload(soundData_);
}

void SceneManager::Initialize(SceneLabel scene, DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
    // 基盤システムのポインタを保存
    dxCommon_ = dxCommon;
    psoManager_ = psoManager;
    textureManager_ = textureManager;
    input_ = input;

    audio_->Initialize();

    // 起動時のシーン判別
    switch (scene) {
    case SceneLabel::Title:       currentScene_ = titleScene_.get(); break;
    case SceneLabel::StageSelect: currentScene_ = stageSelectScene_.get(); break;
    case SceneLabel::Play:        currentScene_ = playScene_.get(); break;
    case SceneLabel::Clear:       currentScene_ = clearScene_.get(); break;
    case SceneLabel::Gameover:    currentScene_ = gameOverScene_.get(); break;
    }

    // BGMの再生
    ChangeBGM(scene);

    // 現在のシーンの初期化
    currentScene_->Initialize(dxCommon_, psoManager_, textureManager_, input_);
}

void SceneManager::Update() {
    currentScene_->Update();

    // シーン切り替えフラグが立っていた場合
    if (currentScene_->GetIsFinish()) {
        SceneLabel nextScene = currentScene_->GetNextScene();

        // 次のシーンインスタンスに切り替え
        switch (nextScene) {
        case SceneLabel::Title:       currentScene_ = titleScene_.get(); break;
        case SceneLabel::StageSelect: currentScene_ = stageSelectScene_.get(); break;
        case SceneLabel::Play:        currentScene_ = playScene_.get(); break;
        case SceneLabel::Clear:       currentScene_ = clearScene_.get(); break;
        case SceneLabel::Gameover:    currentScene_ = gameOverScene_.get(); break;
        }

        // BGMの切り替え（停止・解放・読み込み・再生を一括で行う）
        ChangeBGM(nextScene);

        // 次のシーンを初期化
        currentScene_->Initialize(dxCommon_, psoManager_, textureManager_, input_);
    }
}

void SceneManager::ChangeBGM(SceneLabel scene) {
    // 1. 現在の音を止めてメモリを解放（これをしないと前の音が残り、メモリも食いつぶします）
    audio_->StopSound(soundData_);
    audio_->Unload(soundData_);

    // 2. 次のシーンに応じたパスを選択
    std::string filePath = "";
    switch (scene) {
    case SceneLabel::Title:       filePath = "resources/AL/BGM/title.mp3"; break;
    case SceneLabel::StageSelect: filePath = "resources/AL/BGM/stageSelect.mp3"; break;
    case SceneLabel::Play:        filePath = "resources/AL/BGM/game.mp3"; break;
    case SceneLabel::Clear:       filePath = "resources/AL/BGM/clear.mp3"; break;
    case SceneLabel::Gameover:    filePath = "resources/AL/BGM/gameOver.mp3"; break;
    }

    // 3. ロードして再生
    if (!filePath.empty()) {
        soundData_ = audio_->LoadSound(filePath);
        audio_->PlaySoundW(soundData_, 1.0f, true);
    }
}

void SceneManager::Draw() {
    currentScene_->Draw();
}

void SceneManager::ImGuiControl() {
    ImGui::Begin("Scene");
    switch (currentScene_->GetNowScene()) {
    case SceneLabel::Title:       ImGui::Text("Scene = Title"); break;
    case SceneLabel::StageSelect: ImGui::Text("Scene = StageSelect"); break;
    case SceneLabel::Play:        ImGui::Text("Scene = Play"); break;
    case SceneLabel::Clear:       ImGui::Text("Scene = Clear"); break;
    case SceneLabel::Gameover:    ImGui::Text("Scene = GameOver"); break;
    }
    ImGui::End();
    currentScene_->ImGuiControl();
}
