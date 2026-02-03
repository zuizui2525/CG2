#pragma once
#include "DxCommon.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "Input.h"

enum class SceneLabel {
	Title,
	StageSelect,
	Play,
	Clear,
	Gameover,
};

class Scene {
public:
	virtual ~Scene() = default;

	// 初期化時に基盤システムを渡すように変更
	virtual void Initialize(DxCommon* dxCommon, PSOManager* psoManager, TextureManager* textureManager, Input* input) = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void ImGuiControl() = 0;

	bool GetIsFinish() const { return isFinish_; }
	SceneLabel GetNowScene() const { return nowScene_; }
	SceneLabel GetNextScene() const { return nextScene_; }

	void SetNowScene(SceneLabel scene) { nowScene_ = scene; }

protected:
	bool isFinish_ = false;
	SceneLabel nowScene_;
	SceneLabel nextScene_;
	static inline int selectedStageNum_ = 1;

	// 各シーンで使い回すためのポインタを保持
	DxCommon* dxCommon_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	TextureManager* textureManager_ = nullptr;
	Input* input_ = nullptr;
};
