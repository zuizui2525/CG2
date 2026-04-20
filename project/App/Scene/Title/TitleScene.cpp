#include "App/Scene/Title/TitleScene.h"
#include "Engine/Base/BaseResource.h"
#include "App/Scene/Core/SceneManager.h"

void TitleScene::Initialize() {
    // 1. 各マネージャの取得
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();

    // 2. カメラの生成と登録
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);
    cameraMgr_->SetActiveCamera("Main");

    // 3. ライトの生成（ディレクショナルライト）
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    // 4. モデルのロードと生成（ロード済み）
    bunny_ = std::make_unique<ModelObject>();
    bunny_->Initialize();
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
    // ライトとオブジェクトの更新
    dirLight_->Update();
    bunny_->Update();

    // カメラ本体の更新
    mainCamera_->Update();
}

void TitleScene::Draw() {
    // バニーの描画
    bunny_->Draw("bunny", "bunny");
}
