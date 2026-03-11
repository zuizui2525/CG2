#pragma once
#include "IScene.h"
#include <memory>
#include "ModelObject.h"
#include "LightManager.h"
#include "CameraManager.h"

class TitleScene : public IScene {
public:
    void Initialize() override;
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
