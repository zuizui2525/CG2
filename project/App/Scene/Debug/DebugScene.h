#pragma once
#include "App/Scene/Core/IScene.h"
#include <memory>
#include <string>

class PostProcess;

#include "Engine/Zuizui.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include "Engine/Graphics/Objects/2d/Sprite/SpriteObject.h"

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
    CameraManager* cameraMgr_ = nullptr;
    LightManager* lightMgr_ = nullptr;
    PostProcess* postProcess_ = nullptr;


    // --- ゲームオブジェクト群 ---
    std::shared_ptr<DebugCamera> debugCamera_;
    std::shared_ptr<BaseCamera> mainCamera_;

    std::unique_ptr<DirectionalLightObject> dirLight_;
    std::unique_ptr<SpriteObject> testSprite_;
    std::unique_ptr<SpriteObject> testSprite2_;
};
