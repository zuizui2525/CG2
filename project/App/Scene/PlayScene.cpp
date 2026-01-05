#include "PlayScene.h"

PlayScene::PlayScene() {
}

PlayScene::~PlayScene() {
}

void PlayScene::Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager) {
	// 基盤ポインタの保存
	dxCommon_ = dxCommon;
	psoManager_ = psoManager;
	textureManager_ = textureManager;

	// Camera
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();

	// DirectionalLight
	dirLight_ = std::make_unique<DirectionalLightObject>();
	dirLight_->Initialize(dxCommon_->GetDevice());

	// モデル生成
	teapot_ = std::make_unique<ModelObject>(dxCommon_->GetDevice(), "resources/obj/teapot/teapot.obj", Vector3{ 1.0f, 0.0f, 0.0f });

	// スプライト生成
	sprite_ = std::make_unique<SpriteObject>(dxCommon_->GetDevice(), 640, 360);

	// 球生成
	sphere_ = std::make_unique<SphereObject>(dxCommon_->GetDevice(), 16, 1.0f);

	textureManager_->LoadTexture("teapot", teapot_->GetMaterialResource()->GetGPUVirtualAddress());
	textureManager_->LoadTexture("uvChecker", "resources/uvChecker.png");
	textureManager_->LoadTexture("monsterball", "resources/monsterball.png");
}

void PlayScene::Update() {
	// 本来はInputクラスなども渡す必要があるが、ここでは簡略化
	camera_->Update(nullptr);
	dirLight_->Update();

	// 各オブジェクトの更新
	teapot_->Update(camera_.get());
	sprite_->Update(camera_.get());
	sphere_->Update(camera_.get());
}

void PlayScene::Draw() {
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
