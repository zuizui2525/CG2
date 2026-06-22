#include "App/Scene/Debug/DebugScene.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectFactory.h"
#include "Engine/Graphics/PostProcess/PostProcess.h"


void DebugScene::Initialize() {
    // ポストプロセスのポインタを取得してメンバ変数に保持
    postProcess_ = SceneManager::GetInstance()->GetPostProcess();

    // 各マネージャへのポインタを取得して保持
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // 通常描画用のメインカメラを生成し、カメラマネージャに登録
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);

    // デバッグ用のフリーカメラを生成し、カメラマネージャに登録
    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera("Debug", debugCamera_);
    cameraMgr_->SetActiveCamera("Main");

    // 平行光源の生成と初期化を行い、ライトマネージャに追加
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    // エフェクト管理システムの初期化とエフェクトの全登録
    auto effectMgr = EffectManager::GetInstance();
    effectMgr->Initialize();
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
