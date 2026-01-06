#pragma once
#include "Scene.h"
#include "DirectionalLight.h"
#include "ModelObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "Camera.h"
#include "Skydome.h"
#include <memory>

class TitleScene : public Scene {
public:
	TitleScene();
	~TitleScene() override;

	void Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) override;
	void Update() override;
	void Draw() override;
	void ImGuiControl() override;

private:
	// main.cppから移行した描画オブジェクト
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<DirectionalLightObject> dirLight_;

	// UI
	std::unique_ptr<SpriteObject> title_;
	// skydome
	std::unique_ptr<Skydome> skydome_;

	// 描画フラグ
	bool drawModel_ = true;
	bool drawSprite_ = true;
	bool drawSphere_ = true;
};
