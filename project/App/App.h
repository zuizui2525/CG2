#pragma once
#include <memory>
#include <vector>
#include "Zuizui.h"
#include "Input.h"
#include "CameraManager.h"
#include "LightManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "BaseResource.h"

class App {
public:
	void Initialize();
	void Run();
	void Finalize();

	// メインループの継続判定
	bool IsEnd() const;
private:
    // エンジン・システム
    Zuizui* engine_ = nullptr;

    // マネージャ・リソース
    std::unique_ptr<Input> input_;
    std::unique_ptr<CameraManager> cameraMgr_;
    std::unique_ptr<LightManager> lightMgr_;
    std::unique_ptr<TextureManager> texMgr_;
    std::unique_ptr<ModelManager> modelMgr_;
};

