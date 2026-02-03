#pragma once
#include "Scene.h"
#include "DirectionalLight.h"
#include "ModelObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "Camera.h"
#include "Skydome.h"
#include "Audio.h"
#include <memory>

class StageSelectScene : public Scene {
public:
	StageSelectScene();
	~StageSelectScene() override;

	void Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) override;
	void Update() override;
	void Draw() override;
	void ImGuiControl() override;

private:
	// main.cppから移行した描画オブジェクト
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<DirectionalLightObject> dirLight_;

	// UI
	int stageNum_;
	const char* stageNumName_;
	std::unique_ptr<SpriteObject> stage_;
	// skydome
	std::unique_ptr<Skydome> skydome_;

	std::unique_ptr<Audio> audio_;
	SoundData clickSE_;
	SoundData moveSE_;

	// 描画フラグ
	bool drawModel_ = true;
	bool drawSprite_ = true;
	bool drawSphere_ = true;
};
