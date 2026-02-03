#include "StageSelectScene.h"

StageSelectScene::StageSelectScene() {
}

StageSelectScene::~StageSelectScene() {
	audio_->Unload(clickSE_);
	audio_->Unload(moveSE_);
}

void StageSelectScene::Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
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
	stage_ = std::make_unique<SpriteObject>(dxCommon_->GetDevice(), 1280, 720);
	textureManager_->LoadTexture("stage1", "resources/AL/stage1.png");
	textureManager_->LoadTexture("stage2", "resources/AL/stage2.png");
	textureManager_->LoadTexture("stage3", "resources/AL/stage3.png");

	// Skydomeの生成と初期化
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(dxCommon_, textureManager_);

	stageNum_ = 1;
	stageNumName_ = "stage1";

	audio_ = std::make_unique<Audio>();
	audio_->Initialize();
	clickSE_ = audio_->LoadSound("resources/AL/SE/click.mp3");
	moveSE_ = audio_->LoadSound("resources/AL/SE/move.mp3");
}

void StageSelectScene::Update() {
	camera_->Update(input_);
	dirLight_->Update();

	if (input_->Trigger(DIK_SPACE)) {
		selectedStageNum_ = stageNum_;
		nextScene_ = SceneLabel::Play;
		audio_->PlaySoundW(clickSE_);
		isFinish_ = true;
	}

	if (input_->Trigger(DIK_D)) {
		audio_->PlaySoundW(moveSE_);
		stageNum_++;
	}
	if (input_->Trigger(DIK_A)) {
		audio_->PlaySoundW(moveSE_);
		stageNum_--;
	}

	if (stageNum_ < 1) {
		stageNum_ = 1;
	} else if (stageNum_ > 3) {
		stageNum_ = 3;
	}

	switch (stageNum_) {
	case 1:
		stageNumName_ = "stage1";
		break;
	case 2:
		stageNumName_ = "stage2";
		break;
	case 3:
		stageNumName_ = "stage3";
		break;
	}


	// 各オブジェクトの更新
	stage_->Update(camera_.get());
	skydome_->Update(camera_.get());
}

void StageSelectScene::Draw() {
	// skydomeの描画
	skydome_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());
	// titleの描画
	stage_->Draw(
		dxCommon_->GetCommandList(),
		textureManager_->GetGpuHandle(stageNumName_),
		dirLight_->GetGPUVirtualAddress(),
		psoManager_->GetPSO("Object3D"),
		psoManager_->GetRootSignature("Object3D"),
		drawSprite_
	);
}

void StageSelectScene::ImGuiControl() {
	ImGui::Begin("stage");
	ImGui::Text("stage = [%d]", stageNum_);
	ImGui::End();
}
