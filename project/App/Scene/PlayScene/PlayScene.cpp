#include "PlayScene.h"
#include "Collision.h"

PlayScene::PlayScene() {
}

PlayScene::~PlayScene() {
}

void PlayScene::Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) {
	// 敵のリストをクリアしておく
	enemies_.clear();
	enemySpawnTimer_ = 0.0f;
	// 明示的な開放
	blocks_.clear();
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

	// Clearクラスの生成と初期化
	clear_ = std::make_unique<Clear>();
	clear_->Initialize(dxCommon_, textureManager_, goalPos_);

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
	clear_->Update(camera_.get());
	sprite_->Update(camera_.get());
	sphere_->Update(camera_.get());
	for (auto& block : blocks_) {
		block->Update(camera_.get());
	}

	// --- 1. 敵の出現処理 (2秒ごと) ---
	enemySpawnTimer_ += 1.0f / 60.0f;
	if (enemySpawnTimer_ >= kEnemySpawnInterval) {
		enemySpawnTimer_ = 0.0f;

		auto newEnemy = std::make_unique<Enemy>();

		// --- ここからランダム高さの計算 ---
		float minHeight = 0.2f; // 出現する最小の高さ
		float maxHeight = 2.0f; // 出現する最大の高さ

		// 0.0f ～ 1.0f の間のランダムな小数を生成
		float randomT = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

		// 最小値と最大値の間で補間する (例: 0.2 + (2.0 - 0.2) * randomT)
		float randomY = minHeight + (maxHeight - minHeight) * randomT;
		// ----------------------------------

		newEnemy->Initialize(dxCommon_, textureManager_, { 20.0f, randomY, 0.01f });
		enemies_.push_back(std::move(newEnemy));
	}

	// --- 2. 敵の更新と削除処理 ---
	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		// 敵の移動更新
		(*it)->Update(camera_.get());

		// プレイヤーとの当たり判定 (必要であれば)
		// CheckCollision(player_.get(), it->get());

		// --- マップの端（画面外）に出たかどうかの判定 ---
		Vector3 ePos = (*it)->GetPosition();
		bool isOut = (ePos.x < kMapLeftEnd || ePos.x > kMapRightEnd);

		// 死亡フラグが立っている、または画面外に出たら削除
		if ((*it)->IsDead() || isOut) {
			it = enemies_.erase(it); // リストから削除して次の要素へ
		} else {
			++it;
		}
	}

	// 3. ゴールとの当たり判定（追加）
	Vector3 pPos = player_->GetPosition();
	AABB goalAABB = clear_->GetAABB();

	// プレイヤーがゴールのAABB範囲内に入ったか判定
	if (pPos.x >= goalAABB.min.x && pPos.x <= goalAABB.max.x &&
		pPos.y >= goalAABB.min.y && pPos.y <= goalAABB.max.y) {

		// クリアフラグを立てる
		nextScene_ = SceneLabel::Title;
		isFinish_ = true;
	}

	// 敵の更新と当たり判定
	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		(*it)->Update(camera_.get());

		Enemy::AABB eAABB = (*it)->GetAABB();

		// 簡易AABB判定 (Playerが攻撃中かどうかの判定をPlayerクラスに追加する必要があります)
		if (pPos.x > eAABB.min.x && pPos.x < eAABB.max.x &&
			pPos.y > eAABB.min.y && pPos.y < eAABB.max.y) {

			if (player_->IsAttacking()) {
				// 突進中なら敵を倒す
				(*it)->OnCollisionWithPlayer();
			} else {
				// 通常時ならプレイヤーが死ぬ
				player_->OnCollisionWithEnemy();

				// シーンをタイトルに戻す（またはリトライ）
				nextScene_ = SceneLabel::Title;
				isFinish_ = true;
			}
		}

		// 死亡フラグが立ったらリストから削除
		if ((*it)->IsDead()) {
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}
}

void PlayScene::Draw() {
	// skydomeの描画
	skydome_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());

	// playerの描画
	player_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());

	// clear(ゴール)の描画
	clear_->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());

	// enemyの描画
	for (auto& enemy : enemies_) {
		enemy->Draw(dxCommon_, textureManager_, psoManager_, dirLight_.get());
	}

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
