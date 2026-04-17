#pragma once
#include "App/Scene/IScene.h"
#include <memory>
#include "Engine/Graphics/Objects/3d/Model/ModelObject.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"

class TitleScene : public IScene {
public:
    void Initialize() override;
    void ImGuiControl() override;
    void Update() override;
    void Draw() override;

private:
    // マネージャへのポインタ
    CameraManager* cameraMgr_ = nullptr;
    LightManager* lightMgr_ = nullptr;

    // オブジェクト
    std::shared_ptr<BaseCamera> mainCamera_;
    std::unique_ptr<DirectionalLightObject> dirLight_;
    std::unique_ptr<ModelObject> bunny_;
};
