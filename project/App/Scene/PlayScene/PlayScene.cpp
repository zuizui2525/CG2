#include "PlayScene.h"

PlayScene::PlayScene() {
}

PlayScene::~PlayScene() {
}

void PlayScene::Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
	// 基盤ポインタの保存
	dxCommon_ = dxCommon;
	psoManager_ = psoManager;
	textureManager_ = textureManager;
	input_ = input;
	// シーンのセットと初期化
	nowScene_ = SceneLabel::Play;
	isFinish_ = false;

	// Camera
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();
	camera_->SetTransform({ {1.0f, 1.0f, 1.0f}, {0.2f,0.0f,0.0f}, {0.0f,4.0f,-10.0f} });

	// DirectionalLight
	dirLight_ = std::make_unique<DirectionalLightObject>();
	dirLight_->Initialize(dxCommon_->GetDevice());

	// Skydomeの生成と初期化
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(dxCommon_, textureManager_);

	// mapChipFieldの生成と初期化
	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadMapChipCsv("resources/AL/map/map.csv");
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVertical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x) {
			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipField::MapChipType::kBlock) {
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(x, y);

				// ここで個別に実体を作る
				auto newBlock = std::make_unique<ModelObject>(
					dxCommon_->GetDevice(),
					"resources/AL/cube/cube.obj",
					pos
				);
				newBlock->SetScale({ 0.1f, 0.1f, 0.1f });
				blocks_.push_back(std::move(newBlock));
			}
		}
	}
	if (!blocks_.empty()) {
		textureManager_->LoadTexture("cube", blocks_[0]->GetModelData()->material.textureFilePath);
	}

	// Playerの生成と初期化
	player_ = std::make_unique<Player>();
	player_->Initialize(dxCommon_, textureManager_, input_);
	player_->SetMapChipField(mapChipField_.get());

	// CameraControlの生成と初期化
	cameraControl_ = std::make_unique<CameraControl>();
	cameraControl_->Initialize(camera_.get(), player_.get());

	// スプライト生成
	sprite_ = std::make_unique<SpriteObject>(dxCommon_->GetDevice(), 640, 360);
	textureManager_->LoadTexture("uvChecker", "resources/uvChecker.png");

	// 球生成
	sphere_ = std::make_unique<SphereObject>(dxCommon_->GetDevice(), 16, 1.0f);
	textureManager_->LoadTexture("monsterball", "resources/monsterball.png");
}

void PlayScene::Update() {
	camera_->Update(input_);
	dirLight_->Update();

	if (input_->Trigger(DIK_RETURN)) {
		nextScene_ = SceneLabel::Title;
		isFinish_ = true;
	}

	if (input_->Trigger(DIK_1)) { 
		drawModel_ = true; 
	} else if (input_->Trigger(DIK_2)) {
		drawSprite_ = true;
	} else if (input_->Trigger(DIK_3)) {
		drawSphere_ = true;
	} else if (input_->Trigger(DIK_4)) {
		drawModel_ = false;
		drawSprite_ = false;
		drawSphere_ = false;
	}

	// 各オブジェクトの更新
	skydome_->Update(camera_.get());
	player_->Update(camera_.get());
	cameraControl_->Update();
	sprite_->Update(camera_.get());
	sphere_->Update(camera_.get());
	for (auto& block : blocks_) {
		block->Update(camera_.get());
	}
}

void PlayScene::Draw() {
	// skydomeの描画
	skydome_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());

	// playerの描画
	player_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());

	// マップチップの描画ループ
	for (auto& block : blocks_) {
		block->Draw(
			dxCommon_->GetCommandList(),
			textureManager_->GetGpuHandle("cube"),
			dirLight_->GetGPUVirtualAddress(),
			psoManager_->GetPSO("Object3D"),
			psoManager_->GetRootSignature("Object3D"),
			drawModel_
		);
	}

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

void PlayScene::ImGuiControl() {

}
