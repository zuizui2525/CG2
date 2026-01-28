#pragma once
#include <memory>
#include <vector>
#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "LightManager.h"
#include "Input.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "BaseResource.h"
#include "ModelObject.h"
#include "SphereObject.h"
#include "SpriteObject.h"
#include "TriangleObject.h"
#include "ParticleObject.h"

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
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<DirectionalLightObject> dirLight_;
    std::unique_ptr<LightManager> lightManager_;
    std::unique_ptr<TextureManager> texMgr_;
    std::unique_ptr<ModelManager> modelMgr_;

    // ゲームオブジェクト
    std::unique_ptr<PointLightObject> pointLight_;
    std::unique_ptr<PointLightObject> pointLight2_;
    std::unique_ptr<SpotLightObject> spotLight_;
    std::unique_ptr<ModelObject> skydome_;
    std::unique_ptr<ModelObject> teapot_;
    std::unique_ptr<ModelObject> bunny_;
    std::unique_ptr<ModelObject> terrain_;
    std::unique_ptr<SphereObject> sphere_;
    std::unique_ptr<TriangleObject> triangle_;
    std::unique_ptr<ParticleObject> particle_;
    std::unique_ptr<SpriteObject> sprite_;
};

