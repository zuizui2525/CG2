#include "App/App.h"
#include "App/Scene/Core/SceneManager.h"
#include "App/Scene/Core/SceneFactory.h"
#include "App/Load/ResourceLoader.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"

void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
    engine_->Initialize(L"LE3B_02_イトウカズイ");
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

    // リソースの一括ロード
    ResourceLoader::LoadAll();

    sceneFactory_ = std::make_unique<SceneFactory>();
    SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
    SceneManager::GetInstance()->ChangeScene("Debug");

    // --- PostProcess の初期化 ---
    postProcess_ = std::make_unique<PostProcess>();
    postProcess_->Initialize();
}

void App::Run() {
    // --- ImGui ---
#ifdef _USEIMGUI
    engine_->ImGuiBegin();

    // 全シーン共通のデバッグメニュー
    ImGui::Begin("Scene Manager");
    ImGui::Text("Current Scene: %s", SceneManager::GetInstance()->GetCurrentSceneName().c_str());

    // ポストプロセス 5モード統合切り替えラジオボタン
    PostEffectMode currentMode = postProcess_->GetEffectMode();
    int modeVal = static_cast<int>(currentMode);
    
    ImGui::Separator();
    ImGui::Text("PostEffect Mode:");
    if (ImGui::RadioButton("None (Default Blue)", &modeVal, 0)) { postProcess_->SetEffectMode(PostEffectMode::None); }
    if (ImGui::RadioButton("Red (Debug)", &modeVal, 1)) { postProcess_->SetEffectMode(PostEffectMode::Red); }
    if (ImGui::RadioButton("Black (Debug)", &modeVal, 2)) { postProcess_->SetEffectMode(PostEffectMode::Black); }
    if (ImGui::RadioButton("Grayscale", &modeVal, 3)) { postProcess_->SetEffectMode(PostEffectMode::Grayscale); }
    if (ImGui::RadioButton("Sepia", &modeVal, 4)) { postProcess_->SetEffectMode(PostEffectMode::Sepia); }
    ImGui::Separator();

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

    // 1. ポストプロセス（RenderTexture）への描画パス
    postProcess_->PreDraw();

    SceneManager::GetInstance()->Draw();

    postProcess_->PostDraw();

    // 2. スワップチェーンへのコピー＆転送パス
    postProcess_->Draw();

    engine_->EndFrame();
}

void App::Finalize() {
    SceneManager::GetInstance()->ClearCurrentScene();
    EffectManager::GetInstance()->Finalize();
	engine_->Finalize();
}

bool App::IsEnd() const {
    return !engine_->ProcessMessage();
}
