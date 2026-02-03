#include "TitleScene.h"

TitleScene::TitleScene() {
}

TitleScene::~TitleScene() {
	audio_->Unload(soundData_);
}

void TitleScene::Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
	// 基盤ポインタの保存
	dxCommon_ = dxCommon;
	psoManager_ = psoManager;
	textureManager_ = textureManager;
	input_ = input;
	// シーンのセットと初期化
	nowScene_ = SceneLabel::Title;
	isFinish_ = false;

	// Camera
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();
	camera_->SetTransform({ {1.0f, 1.0f, 1.0f}, {0.2f,0.0f,0.0f}, {0.0f,4.0f,-20.0f} });

	// DirectionalLight
	dirLight_ = std::make_unique<DirectionalLightObject>();
	dirLight_->Initialize(dxCommon_->GetDevice());

	// スプライト生成
	title_ = std::make_unique<SpriteObject>(dxCommon_->GetDevice(), 1280, 720);
	textureManager_->LoadTexture("title", "resources/AL/title.png");

	// Skydomeの生成と初期化
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(dxCommon_, textureManager_);

	audio_ = std::make_unique<Audio>();
	audio_->Initialize();
	soundData_ = audio_->LoadSound("resources/AL/SE/click.mp3");
}

void TitleScene::Update() {
	camera_->Update(input_);
	dirLight_->Update();

	if (input_->Trigger(DIK_SPACE)) {
		nextScene_ = SceneLabel::StageSelect;
		audio_->PlaySoundW(soundData_);
		isFinish_ = true;
	}

	// 各オブジェクトの更新
	title_->Update(camera_.get());
	skydome_->Update(camera_.get());
}

void TitleScene::Draw() {
	// skydomeの描画
	skydome_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());
	// titleの描画
	title_->Draw(
		dxCommon_->GetCommandList(),
		textureManager_->GetGpuHandle("title"),
		dirLight_->GetGPUVirtualAddress(),
		psoManager_->GetPSO("Object3D"),
		psoManager_->GetRootSignature("Object3D"),
		drawSprite_
	);
}

void TitleScene::ImGuiControl() {

}
