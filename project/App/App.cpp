#include "App/App.h"
#include "Engine/Debug/DebugEditor.h"
#include "Engine/Debug/ReplaySystem.h"
#include "App/Scene/Core/SceneManager.h"
#include "App/Scene/Core/SceneFactory.h"
#include "App/Load/ResourceLoader.h"
#include "Engine/Debug/SceneHierarchy.h"
#include "Engine/Graphics/Objects/3d/Object3D.h"
#include "Engine/Graphics/Objects/2d/Sprite/SpriteObject.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include "Engine/Base/Log/Log.h"
#include <psapi.h> // メモリ取得用（追加）

#pragma comment(lib, "psapi.lib") // 追加



void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
    EngineResource::SetEngine(engine_);
#ifdef _USEIMGUI
    engine_->Initialize(L"ZuizuiEngine");
#else
    engine_->Initialize(L"LE3B_02_イトウカズイ");
#endif

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
    static auto lastTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    if (deltaTime < 0.0001f) { deltaTime = 0.0001f; }
    if (deltaTime > 1.0f) { deltaTime = 1.0f; }
    float currentFps = 1.0f / deltaTime;

    static float cachedMem = 0.0f;
    static float memTimer = 0.0f;
    memTimer -= deltaTime;
    if (memTimer <= 0.0f || cachedMem == 0.0f) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            cachedMem = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
        }
        memTimer = 0.5f; // 0.5秒ごとに更新
    }
    float currentMem = cachedMem;

    // 高負荷（スパイク）警告ログの出力処理（マジックナンバー排除）
    static float spikeWarningCooldown = 0.0f;
    if (spikeWarningCooldown > 0.0f) {
        spikeWarningCooldown -= deltaTime;
    }
    constexpr float kSpikeFpsThreshold = 30.0f;
    constexpr float kSpikeWarningCooldownMax = 5.0f; // クールタイムは5秒間

    if (currentFps < kSpikeFpsThreshold && spikeWarningCooldown <= 0.0f) {
        Log::Write(std::format("[警告] ★高負荷スパイク検知: FPSが一時的に低下しました ({:.1f} FPS) | 物理メモリ使用量: {:.2f} MB | フレーム時間: {:.4f} 秒", currentFps, currentMem, deltaTime));
        spikeWarningCooldown = kSpikeWarningCooldownMax;
    }

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

    // ポーズのON/OFFに応じてカメラを自動で切り替える
    static bool sWasPaused = false;
    static std::string sPrevActiveCamera = "";

    if (isPaused != sWasPaused) {
        if (isPaused) {
            // ポーズになった瞬間に、現在アクティブなカメラの名前を記憶し、"Editor" をアクティブにする
            sPrevActiveCamera = cameraMgr_->GetActiveCameraName();
            if (cameraMgr_->HasCamera("Editor")) {
                cameraMgr_->SetActiveCamera("Editor");
                if (auto* dc = dynamic_cast<DebugCamera*>(cameraMgr_->GetActiveCamera())) {
                    dc->SetActive(true);
                }
            }
        } else {
            // ポーズが解除された瞬間に、記憶していたカメラに戻す
            if (!sPrevActiveCamera.empty() && cameraMgr_->HasCamera(sPrevActiveCamera)) {
                if (auto* dc = dynamic_cast<DebugCamera*>(cameraMgr_->GetActiveCamera())) {
                    dc->SetActive(false);
                }
                cameraMgr_->SetActiveCamera(sPrevActiveCamera);
            }
        }
        sWasPaused = isPaused;
    }

    // --- 更新 ---
    input_->Update();
    
    if (!isPaused) {
        cameraMgr_->Update();
        lightMgr_->Update();
        Log::Update(deltaTime);
        SceneManager::GetInstance()->Update();
    } else {
        // ポーズ中の更新処理：
        // Game View が表示されている場合のみ、オブジェクト編集やカメラ見回しを反映させる
        if (isGameViewVisible) {
            cameraMgr_->Update();
            lightMgr_->Update();

            // もしアクティブカメラが DebugCamera であれば、ポーズ中も入力操作で飛び回れるように更新する
            BaseCamera* activeCam = cameraMgr_->GetActiveCamera();
            if (auto* dc = dynamic_cast<DebugCamera*>(activeCam)) {
                dc->Update(input_.get());
            }

            // シーン内のすべてのオブジェクトを更新して行列再計算（位置・色変更）を即座に反映させる
            const auto& objects = SceneHierarchy::GetInstance()->GetObjects();
            for (auto* obj : objects) {
                if (obj) {
                    obj->Update();
                }
            }
        }
    }


    // --- 描画 ---
    engine_->BeginFrame();

    // ポーズ中かつ Game View が非表示の場合は、本編 3D レンダリングを完全にスキップして負荷を最小にする
    bool shouldDraw = !(isPaused && !isGameViewVisible);

    if (shouldDraw) {
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
    }

#ifdef _USEIMGUI
    // 一時停止中でなく、かつリプレイ機能が有効な場合のみリプレイバッファを記録
    bool isReplayEnabled = true;
    if (auto debugEditor = engine_->GetDebugEditor()) {
        isReplayEnabled = debugEditor->IsReplayEnabled();
    }

    if (!isPaused && isReplayEnabled) {
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
