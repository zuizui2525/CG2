#include "App/App.h"
#include "Engine/Debug/DebugEditor.h"
#include "Engine/Debug/ReplaySystem.h"
#include "App/Scene/Core/SceneManager.h"
#include "App/Scene/Core/SceneFactory.h"
#include "App/Load/ResourceLoader.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include "Engine/Base/Log/Log.h"
#include <psapi.h> // メモリ取得用（追加）

#pragma comment(lib, "psapi.lib") // 追加



void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
#ifdef _USEIMGUI
    engine_->Initialize(L"ZuizuiEngine");
#else
    engine_->Initialize(L"LE3B_02_イトウカズイ");
#endif
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
    SceneManager::GetInstance()->ChangeScene("Title");

    // --- PostProcess の初期化 ---
    postProcess_ = std::make_unique<PostProcess>();
    postProcess_->Initialize();

    SceneManager::GetInstance()->SetPostProcess(postProcess_.get());
}

void App::Run() {
#ifdef _USEIMGUI
    ReplaySystem::GetInstance()->ClearGarbage();
#endif

    // 現在のウィンドウの実際のクライアント領域サイズを取得し、サイズ変更を検知
    HWND hwnd = engine_->GetWindow()->GetHWND();
    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);
    int32_t currentWidth = clientRect.right - clientRect.left;
    int32_t currentHeight = clientRect.bottom - clientRect.top;

    static int32_t lastWidth = currentWidth;
    static int32_t lastHeight = currentHeight;

    if (currentWidth > 0 && currentHeight > 0 && 
        (currentWidth != lastWidth || currentHeight != lastHeight)) {
        
        // 1. スワップチェーンと深度バッファのリサイズ
        engine_->GetDxCommon()->ResizeSwapChain(currentWidth, currentHeight);

        // 2. ポストプロセスレンダーテクスチャのリサイズ
        postProcess_->Resize(currentWidth, currentHeight);

        // 3. カメラのプロジェクションアスペクト比の動的更新
        float aspect = static_cast<float>(currentWidth) / static_cast<float>(currentHeight);
        cameraMgr_->UpdateAllProjection(aspect);

#ifdef _USEIMGUI
        // 4. リプレイシステムのリサイズ通知
        ReplaySystem::GetInstance()->OnResize(currentWidth, currentHeight);
#endif

        lastWidth = currentWidth;
        lastHeight = currentHeight;
    }

    // FPSおよび物理メモリの計測
    float currentMem = 0.0f;
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        currentMem = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
    }

    static auto lastTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    if (deltaTime < 0.0001f) { deltaTime = 0.0001f; }
    if (deltaTime > 1.0f) { deltaTime = 1.0f; }
    float currentFps = 1.0f / deltaTime;

    bool isGameViewVisible = false;
    bool isPaused = false;

#ifdef _USEIMGUI
    isPaused = ReplaySystem::GetInstance()->IsPaused();
#endif

    // --- ImGui ---
#ifdef _USEIMGUI
    engine_->ImGuiBegin();
    
    // ゲーム側（シーン固有）のデバッグUIを描画
    SceneManager::GetInstance()->ImGuiControl();
    
    engine_->ImGuiEnd();
    if (auto debugEditor = engine_->GetDebugEditor()) {
        isGameViewVisible = debugEditor->IsGameViewVisible();
    }
#endif

    // --- 更新 ---
    input_->Update();
    cameraMgr_->Update();
    lightMgr_->Update();
    
    if (!isPaused) {
        Log::Update(deltaTime);
        SceneManager::GetInstance()->Update();
    }


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

#ifdef _USEIMGUI
    // 一時停止中でない場合のみリプレイバッファを記録
    if (!isPaused) {
        ReplaySystem::GetInstance()->RecordFrame(
            engine_->GetDxCommon()->GetCommandList(),
            postProcess_->GetFinalResource(),
            postProcess_->GetFinalSrvGpuHandle(),
            currentFps,
            currentMem
        );
    }

#endif

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
