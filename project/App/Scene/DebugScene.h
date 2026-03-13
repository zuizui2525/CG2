#pragma once
#include "IScene.h"
#include <memory>
#include <string>

#include "Zuizui.h"
#include "BaseResource.h"
#include "Input.h"
#include "CameraManager.h"
#include "DebugCamera.h"
#include "LightManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ModelObject.h"
#include "SphereObject.h"
#include "SpriteObject.h"
#include "TriangleObject.h"
#include "ParticleObject.h"

#ifdef _USEIMGUI
#include "imguiManager.h"
#endif

class DebugScene : public IScene {
public:
    // 初期化・更新・描画のオーバーライド
    void Initialize() override;
    void ImGuiControl() override;
    void Update() override;
    void Draw() override;

private:
    // --- 各システムマネージャへのポインタ ---
    Input* input_ = nullptr;
    TextureManager* texMgr_ = nullptr;
    ModelManager* modelMgr_ = nullptr;
    CameraManager* cameraMgr_ = nullptr;
    LightManager* lightMgr_ = nullptr;

    // --- ゲームオブジェクト群 ---
    std::shared_ptr<DebugCamera> debugCamera_;
    std::shared_ptr<BaseCamera> mainCamera_;

    std::unique_ptr<DirectionalLightObject> dirLight_;
    std::unique_ptr<DirectionalLightObject> dirLight2_;
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
