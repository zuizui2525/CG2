#include "App.h"
#include "SceneManager.h"
#include "SceneFactory.h"

void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
    engine_->Initialize(L"LE2B_02_イトウカズイ");
    EngineResource::SetEngine(engine_);

    input_ = std::make_unique<Input>();
    input_->Initialize();
    InputResource::SetInput(input_.get());

    cameraMgr_ = std::make_unique<CameraManager>();
    cameraMgr_->Initialize();
    CameraResource::SetCameraManager(cameraMgr_.get());

    lightMgr_ = std::make_unique<LightManager>();
    lightMgr_->Initialize();
    LightResource::SetLightManager(lightMgr_.get());

    texMgr_ = std::make_unique<TextureManager>();
    texMgr_->Initialize();
    TextureResource::SetTextureManager(texMgr_.get());

    modelMgr_ = std::make_unique<ModelManager>();
    modelMgr_->Initialize();
    ModelResource::SetModelManager(modelMgr_.get());

    sceneFactory_ = std::make_unique<SceneFactory>();
    SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
    SceneManager::GetInstance()->ChangeScene("Debug");
}

void App::Run() {
    // --- ImGui ---
#ifdef _USEIMGUI
    engine_->ImGuiBegin();

    // 全シーン共通のデバッグメニュー
    ImGui::Begin("Scene Manager");
    ImGui::Text("Current Scene: %s", SceneManager::GetInstance()->GetCurrentSceneName().c_str());
    if (ImGui::Button("Reset DebugScene")) {
        SceneManager::GetInstance()->ChangeScene("Debug");
    }
    if (ImGui::Button("Reset TitleScene")) {
        SceneManager::GetInstance()->ChangeScene("Title");
    }
    ImGui::End();

    SceneManager::GetInstance()->ImGuiControl();

    engine_->ImGuiEnd();
#endif

    // --- 更新 ---
    input_->Update();
    cameraMgr_->Update();
    lightMgr_->Update();

    SceneManager::GetInstance()->Update();

    // --- 描画 ---
    engine_->BeginFrame();

    SceneManager::GetInstance()->Draw();

    engine_->EndFrame();
}

void App::Finalize() {
    SceneManager::GetInstance()->ClearCurrentScene();
	engine_->Finalize();
}

bool App::IsEnd() const {
    return !engine_->ProcessMessage();
}
