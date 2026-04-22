#pragma once
#include "App/Scene/Core/IScene.h"
#include <memory>

#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/3d/Sphere/SphereObject.h"
#include "Engine/Graphics/Objects/3d/Model/ModelObject.h"
#include "Engine/Graphics/Objects/3d/Skybox/Skybox.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"

class TitleScene : public IScene {
public:
    void Initialize() override;
    void ImGuiControl() override;
    void Update() override;
    void Draw() override;

private:
    // マネージャへのポインタ
    Input* input_ = nullptr;
    CameraManager* cameraMgr_ = nullptr;
    LightManager* lightMgr_ = nullptr;
    
    // オブジェクト
    std::shared_ptr<BaseCamera> mainCamera_;
    std::shared_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<DirectionalLightObject> dirLight_;
    std::unique_ptr<SphereObject> sphere_;
    std::unique_ptr<ModelObject> bunny_;
    std::unique_ptr<Skybox> skybox_;
};
