#pragma once
#include "Scene.h"
#include "DirectionalLight.h"
#include "ModelObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "Camera.h"
#include <memory>

class PlayScene : public Scene {
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager) override;
	void Update() override;
	void Draw() override;

private:
	// main.cppから移行した描画オブジェクト
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<DirectionalLightObject> dirLight_;
	std::unique_ptr<ModelObject> teapot_;
	std::unique_ptr<SpriteObject> sprite_;
	std::unique_ptr<SphereObject> sphere_;

	// 描画フラグ（ImGuiで制御していたもの）
	bool drawModel_ = true;
	bool drawSprite_ = false;
	bool drawSphere_ = false;
};
