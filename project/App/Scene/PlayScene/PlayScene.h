#pragma once
#include "Scene.h"
#include "DirectionalLight.h"
#include "ModelObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "Camera.h"
#include "MapChipField.h"
#include "Skydome.h"
#include "player.h"
#include "CameraControl.h"
#include "Clear.h"
#include "Enemy.h"
#include <memory>
#include <list>

class PlayScene : public Scene {
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) override;
	void Update() override;
	void Draw() override;
	void ImGuiControl() override;

private:
	// main.cppから移行した描画オブジェクト
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<CameraControl> cameraControl_;
	std::unique_ptr<DirectionalLightObject> dirLight_;

	std::unique_ptr<SpriteObject> sprite_;
	
	// skydome
	std::unique_ptr<Skydome> skydome_;

	// player
	std::unique_ptr<Player> player_;
	
	// enemy
	std::list<std::unique_ptr<Enemy>> enemies_;

	// 出現タイマー
	float enemySpawnTimer_ = 0.0f;
	static inline const float kEnemySpawnInterval = 1.0f; // 1秒間隔

	// マップの端の定義 (例: X座標が-5〜25の範囲外に出たら消す)
	static inline const float kMapLeftEnd = -2.0f;
	static inline const float kMapRightEnd = 25.0f;

	// Clear
	std::unique_ptr<Clear> clear_;
	Vector3 goalPos_ = { 19.4f, 1.0f, 0.0f };

	// mapChipField
	std::unique_ptr<MapChipField> mapChipField_;
	std::vector<std::unique_ptr<ModelObject>> blocks_;

	// 描画フラグ
	bool drawModel_ = true;
};
