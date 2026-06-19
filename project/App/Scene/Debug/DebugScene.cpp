#include "App/Scene/Debug/DebugScene.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectFactory.h"
#include "Engine/Graphics/PostProcess/PostProcess.h"


void DebugScene::Initialize() {
    // 0. ポストプロセスのポインタを取得してメンバ変数に保持
    postProcess_ = SceneManager::GetInstance()->GetPostProcess();

    // 1. 各マネージャのポインタを取得して保持する
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // 2. カメラの生成と設定
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);

    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera("Debug", debugCamera_);
    cameraMgr_->SetActiveCamera("Main");

    // 3. ライトの生成
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    auto effectMgr = EffectManager::GetInstance();
    effectMgr->Initialize();

    // エフェクトの一括登録
    EffectFactory::GetInstance()->RegisterAllEffects();
}

void DebugScene::ImGuiControl() {
#ifdef _USEIMGUI
    cameraMgr_->ImGuiControl();
    EffectManager::GetInstance()->ImGuiControl("Effects");
    if (postProcess_) {
        postProcess_->ImGuiControl();
    }
#endif
}

void DebugScene::Update() {
    // シーン切り替え
    if (input_->Trigger(DIK_N)) {
        SceneManager::GetInstance()->ChangeScene("Title");
    }

#ifdef _USEIMGUI
    // モード切り替え（TABキー）
    if (input_->Trigger(DIK_TAB)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? "Main" : "Debug");
    }
#endif

    // オブジェクトの更新
    lightMgr_->Update();
    dirLight_->Update();
    
    EffectManager::GetInstance()->Update();
    
    // カメラの更新
    BaseCamera* active = cameraMgr_->GetActiveCamera();
    DebugCamera* dc = dynamic_cast<DebugCamera*>(active);

    if (dc) {
        dc->SetActive(true);
        dc->Update(input_);
    } else {
        debugCamera_->SetActive(false);
        active->Update();
    }
}

void DebugScene::Draw() {
    EffectManager::GetInstance()->Draw();
}
