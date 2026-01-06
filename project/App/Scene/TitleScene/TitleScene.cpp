#include "TitleScene.h"

TitleScene::TitleScene() {
}

TitleScene::~TitleScene() {
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

	// モデル生成
	teapot_ = std::make_unique<ModelObject>(dxCommon_->GetDevice(), "resources/obj/teapot/teapot.obj", Vector3{ 1.0f, 0.0f, 0.0f });
	textureManager_->LoadTexture("teapot", teapot_->GetModelData()->material.textureFilePath);

	// スプライト生成
	sprite_ = std::make_unique<SpriteObject>(dxCommon_->GetDevice(), 640, 360);
	textureManager_->LoadTexture("uvChecker", "resources/uvChecker.png");

	// 球生成
	sphere_ = std::make_unique<SphereObject>(dxCommon_->GetDevice(), 16, 1.0f);
	textureManager_->LoadTexture("monsterball", "resources/monsterball.png");
}

void TitleScene::Update() {
	camera_->Update(input_);
	dirLight_->Update();

	if (input_->Trigger(DIK_SPACE)) {
		nextScene_ = SceneLabel::Play;
		isFinish_ = true;
	}

	// 各オブジェクトの更新
	teapot_->Update(camera_.get());
	sprite_->Update(camera_.get());
	sphere_->Update(camera_.get());
}

void TitleScene::Draw() {
	// Modelの描画
	teapot_->Draw(
		dxCommon_->GetCommandList(),
		textureManager_->GetGpuHandle("teapot"),
		dirLight_->GetGPUVirtualAddress(),
		psoManager_->GetPSO("Object3D"),
		psoManager_->GetRootSignature("Object3D"),
		drawModel_
	);

	// Spriteの描画
	sprite_->Draw(
		dxCommon_->GetCommandList(),
		textureManager_->GetGpuHandle("uvChecker"),
		dirLight_->GetGPUVirtualAddress(),
		psoManager_->GetPSO("Object3D"),
		psoManager_->GetRootSignature("Object3D"),
		drawSprite_
	);

	// Sphereの描画
	sphere_->Draw(
		dxCommon_->GetCommandList(),
		textureManager_->GetGpuHandle("monsterball"),
		dirLight_->GetGPUVirtualAddress(),
		psoManager_->GetPSO("Object3D"),
		psoManager_->GetRootSignature("Object3D"),
		drawSphere_
	);
}

void TitleScene::ImGuiControl() {

}
