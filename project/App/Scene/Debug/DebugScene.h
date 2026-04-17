#pragma once
#include "App/Scene/Core/IScene.h"
#include <memory>
#include <string>

#include "Engine/Zuizui.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/3d/Model/ModelManager.h"
#include "Engine/Graphics/Objects/3d/Model/ModelObject.h"
#include "Engine/Graphics/Objects/3d/Sphere/SphereObject.h"
#include "Engine/Graphics/Objects/2d/Sprite/SpriteObject.h"
#include "Engine/Graphics/Objects/3d/Triangle/TriangleObject.h"
#include "Engine/Graphics/Objects/3d/Particle/ParticleObject.h"

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
