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
#include <memory>

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
	std::unique_ptr<SphereObject> sphere_;

	// skydome
	std::unique_ptr<Skydome> skydome_;

	// player
	std::unique_ptr<Player> player_;
	
	// Clear
	std::unique_ptr<Clear> clear_;
	Vector3 goalPos_ = { 19.6f, 1.0f, 0.0f };

	// mapChipField
	std::unique_ptr<MapChipField> mapChipField_;
	std::vector<std::unique_ptr<ModelObject>> blocks_;

	// 描画フラグ
	bool drawModel_ = true;
	bool drawSprite_ = false;
	bool drawSphere_ = false;
};
