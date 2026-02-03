#include "GameOverScene.h"

GameOverScene::GameOverScene() {
}

GameOverScene::~GameOverScene() {
}

void GameOverScene::Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
	// 基盤ポインタの保存
	dxCommon_ = dxCommon;
	psoManager_ = psoManager;
	textureManager_ = textureManager;
	input_ = input;
	// シーンのセットと初期化
	nowScene_ = SceneLabel::Gameover;
	isFinish_ = false;

	// Camera
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();
	camera_->SetTransform({ {1.0f, 1.0f, 1.0f}, {0.2f,0.0f,0.0f}, {0.0f,4.0f,-20.0f} });

	// DirectionalLight
	dirLight_ = std::make_unique<DirectionalLightObject>();
	dirLight_->Initialize(dxCommon_->GetDevice());

	// スプライト生成
	gameOver_ = std::make_unique<SpriteObject>(dxCommon_->GetDevice(), 1280, 720);
	textureManager_->LoadTexture("gameOver", "resources/AL/gameOver.png");

	// Skydomeの生成と初期化
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(dxCommon_, textureManager_);
}

void GameOverScene::Update() {
	camera_->Update(input_);
	dirLight_->Update();

	if (input_->Trigger(DIK_SPACE)) {
		nextScene_ = SceneLabel::Title;
		isFinish_ = true;
	}

	// 各オブジェクトの更新
	gameOver_->Update(camera_.get());
	skydome_->Update(camera_.get());
}

void GameOverScene::Draw() {
	// skydomeの描画
	skydome_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());
	// titleの描画
	gameOver_->Draw(
		dxCommon_->GetCommandList(),
		textureManager_->GetGpuHandle("gameOver"),
		dirLight_->GetGPUVirtualAddress(),
		psoManager_->GetPSO("Object3D"),
		psoManager_->GetRootSignature("Object3D"),
		drawSprite_
	);
}

void GameOverScene::ImGuiControl() {

}
