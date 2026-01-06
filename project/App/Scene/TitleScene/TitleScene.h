#pragma once
#include "Scene.h"
#include "DirectionalLight.h"
#include "ModelObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "Camera.h"
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

	std::unique_ptr<ModelObject> teapot_;
	Vector3 position_{};
	float speed_ = 0.1f;
	std::unique_ptr<SpriteObject> sprite_;
	std::unique_ptr<SphereObject> sphere_;

	// 描画フラグ
	bool drawModel_ = true;
	bool drawSprite_ = true;
	bool drawSphere_ = true;
};
