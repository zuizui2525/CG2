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

    SceneManager::GetInstance()->SetPostProcess(postProcess_.get());
}

void App::Run() {
    bool isGameViewVisible = false;

    // --- ImGui ---
#ifdef _USEIMGUI
    engine_->ImGuiBegin();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // Game View ウィンドウ (UnityのGameビューのようにImGui内にゲーム画面を描画)
    static bool showGameView = true;
    if (showGameView) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin("Game View", &showGameView)) {
            isGameViewVisible = true;
            
            // ウィンドウサイズを取得し、アスペクト比を維持したサイズを計算
            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            
            // アスペクト比（16:9）を維持（マジックナンバーを避けるための定数定義）
            const float kAspectWidth = 16.0f;
            const float kAspectHeight = 9.0f;
            const float kAspectRatio = kAspectWidth / kAspectHeight;
            
            float width = contentSize.x;
            float height = contentSize.x / kAspectRatio;
            
            if (height > contentSize.y) {
                height = contentSize.y;
                width = contentSize.y * kAspectRatio;
            }
            
            // 中央揃え用のパディング計算
            ImVec2 cursorPadding = ImVec2(
                (contentSize.x - width) * 0.5f,
                (contentSize.y - height) * 0.5f
            );
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + cursorPadding.x, ImGui::GetCursorPosY() + cursorPadding.y));
            
            // ポストプロセスの最終結果テクスチャを描画
            D3D12_GPU_DESCRIPTOR_HANDLE finalSrv = postProcess_->GetFinalSrvGpuHandle();
            ImTextureID texID = (ImTextureID)finalSrv.ptr;
            
            ImGui::Image(texID, ImVec2(width, height));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // Console ウィンドウの描画
    Log::DrawConsoleWindow();

    // 全シーン共通のデバッグメニュー
    ImGui::Begin("Scene Manager");
    ImGui::Text("Current Scene: %s", SceneManager::GetInstance()->GetCurrentSceneName().c_str());

    // マジックストリング回避のためのローカル定数定義
    static const std::string kDebugSceneName = "Debug";
    static const std::string kTitleSceneName = "Title";
    static const std::string kGameSceneName = "Game";
    static const std::string kClearSceneName = "Clear";
    static const std::string kGameOverSceneName = "GameOver";

    if (ImGui::Button("Reset DebugScene")) {
        SceneManager::GetInstance()->ChangeScene(kDebugSceneName);
    }
    if (ImGui::Button("Reset TitleScene")) {
        SceneManager::GetInstance()->ChangeScene(kTitleSceneName);
    }
    if (ImGui::Button("Reset GameScene")) {
        SceneManager::GetInstance()->ChangeScene(kGameSceneName);
    }
    if (ImGui::Button("Reset ClearScene")) {
        SceneManager::GetInstance()->ChangeScene(kClearSceneName);
    }
    if (ImGui::Button("Reset GameOverScene")) {
        SceneManager::GetInstance()->ChangeScene(kGameOverSceneName);
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

    // 2. エフェクト適用およびスワップチェーンへのコピー処理
    if (isGameViewVisible) {
        // Game Viewが表示されている場合、エフェクト適用のみを行い、スワップチェーンへのコピーは不要
        postProcess_->ProcessEffects();
    } else {
        // Game Viewが表示されていない場合、通常どおりエフェクト適用＆スワップチェーン全体への描画コピーを行う
        postProcess_->Draw();
    }

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
