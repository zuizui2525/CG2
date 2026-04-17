#pragma once
#include <memory>
#include <vector>
#include "Engine/Zuizui.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/3d/Model/ModelManager.h"
#include "Engine/Base/BaseResource.h"
#include "App/Scene/AbstractSceneFactory.h"

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
    std::unique_ptr<AbstractSceneFactory> sceneFactory_;
};

