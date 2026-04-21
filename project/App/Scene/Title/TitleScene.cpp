#include "App/Scene/Title/TitleScene.h"
#include "Engine/Base/BaseResource.h"
#include "App/Scene/Core/SceneManager.h"

void TitleScene::Initialize() {
    // 1. 各マネージャの取得
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // 2. カメラの生成と登録
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);
    cameraMgr_->SetActiveCamera("Main");

    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera("Debug", debugCamera_);
    cameraMgr_->SetActiveCamera("Main");

    // 3. ライトの生成（ディレクショナルライト）
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    // 4. モデルの生成（ロード済み）
    bunny_ = std::make_unique<ModelObject>();
    bunny_->Initialize();

    // 5. Skyboxの生成
    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize();
}

void TitleScene::ImGuiControl() {
#ifdef _USEIMGUI
    // シーン内のオブジェクトのデバッグ表示
    cameraMgr_->ImGuiControl();
    dirLight_->ImGuiControl("dirLight");
    bunny_->ImGuiControl("bunny");
#endif
}

void TitleScene::Update() {
    // シーン切り替え
    if (input_->Trigger(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene("Debug");
    }

    // モード切り替え（TABキー）
    if (input_->Trigger(DIK_TAB)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? "Main" : "Debug");
    }

    // ライトとオブジェクトの更新
    dirLight_->Update();
    bunny_->Update();
    skybox_->Update();

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

void TitleScene::Draw() {
    // Skyboxの描画（透過を含まない他のモデルより先、または後に描画）
    skybox_->Draw("skyboxTex");

    // バニーの描画
    bunny_->Draw("bunny", "bunny");
}
