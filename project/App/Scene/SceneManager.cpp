#include "SceneManager.h"

SceneManager::SceneManager() {
    scene_ = SceneLabel::Play;
    playScene_ = std::make_unique<PlayScene>();
    currentScene_ = playScene_.get();
}

SceneManager::~SceneManager() {}

void SceneManager::Initialize(SceneLabel scene, DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager) {
    // 基盤システムのポインタを保存しておく
    dxCommon_ = dxCommon;
    psoManager_ = psoManager;
    textureManager_ = textureManager;

    // 引数で初期化のシーンを選択
    switch (scene) {
    case SceneLabel::Play:
        currentScene_ = playScene_.get();
        break;
    }

    // 現在のシーンに基盤を渡して初期化
    currentScene_->Initialize(dxCommon_, psoManager_, textureManager_);
}

void SceneManager::Update() {
    currentScene_->Update();

    // シーン切り替えフラグが立っていた場合
    if (currentScene_->GetIsFinish()) {
        switch (currentScene_->GetNextScene()) {
        case SceneLabel::Play:
            currentScene_ = playScene_.get();
            break;
            // 他のシーンへの切り替え処理...
        }

        // 次のシーンも同じ基盤システムで初期化する
        currentScene_->Initialize(dxCommon_, psoManager_, textureManager_);
    }
}

void SceneManager::Draw() {
    currentScene_->Draw();
}
