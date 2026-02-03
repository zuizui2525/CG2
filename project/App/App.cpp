#include "App.h"

void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
    engine_->Initialize(L"LE2B_02_イトウカズイ");
    EngineResource::SetEngine(engine_);

    input_ = std::make_unique<Input>();
    input_->Initialize(engine_->GetWindow()->GetInstance(), engine_->GetWindow()->GetHWND());

    cameraMgr_ = std::make_unique<CameraManager>();
    cameraMgr_->Initialize(engine_->GetDevice());
    CameraResource::SetCameraManager(cameraMgr_.get());

    lightMgr_ = std::make_unique<LightManager>();
    lightMgr_->Initialize();
    LightResource::SetLightManager(lightMgr_.get());

    texMgr_ = std::make_unique<TextureManager>();
    texMgr_->Initialize(engine_->GetDevice(), engine_->GetDxCommon()->GetCommandList(), engine_->GetDxCommon()->GetSrvHeap());
    TextureResource::SetTextureManager(texMgr_.get());

    modelMgr_ = std::make_unique<ModelManager>();
    modelMgr_->Initialize(engine_->GetDevice(), texMgr_.get());
    ModelResource::SetModelManager(modelMgr_.get());

    // テクスチャ
    texMgr_->LoadTexture("white", "resources/white.png");
    texMgr_->LoadTexture("monsterBall", "resources/monsterBall.png");
    texMgr_->LoadTexture("circle", "resources/circle.png");
    texMgr_->LoadTexture("uvChecker", "resources/uvChecker.png");

    // モデル
    modelMgr_->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
    modelMgr_->LoadModel("bunny", "resources/obj/bunny/bunny.obj");
    modelMgr_->LoadModel("terrain", "resources/obj/terrain/terrain.obj");
    modelMgr_->LoadModel("skydome", "resources/obj/skydome/skydome.obj");

    // ゲームオブジェクト
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);

    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    debugCamera_->SetHwnd(engine_->GetWindow()->GetHWND());
    cameraMgr_->AddCamera("Debug", debugCamera_);

    // 初期カメラの設定
    cameraMgr_->SetActiveCamera("Main");

    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    dirLight2_ = std::make_unique<DirectionalLightObject>();
    dirLight2_->Initialize();
    dirLight2_->GetLightData().color = { 1.0f, 0.0f, 0.0f, 1.0f };
    dirLight2_->GetLightData().direction = { 1.0f, -1.0f, 0.0f };
    lightMgr_->AddDirectionalLight(dirLight2_.get());

    pointLight_ = std::make_unique<PointLightObject>();
    pointLight_->Initialize();
    lightMgr_->AddPointLight(pointLight_.get());

    pointLight2_ = std::make_unique<PointLightObject>();
    pointLight2_->Initialize();
    pointLight2_->SetPosition({ -5.0f, 2.0f, 0.0f });
    lightMgr_->AddPointLight(pointLight2_.get());

    spotLight_ = std::make_unique<SpotLightObject>();
    spotLight_->Initialize();
    lightMgr_->AddSpotLight(spotLight_.get());

    skydome_ = std::make_unique<ModelObject>();
    skydome_->Initialize();
    skydome_->SetLightingMode(0);
    skydome_->SetPosition({ 0.0f, 0.0f, 0.0f });

    teapot_ = std::make_unique<ModelObject>();
    teapot_->Initialize();
    teapot_->SetPosition({ 0.0f, 0.0f, 0.0f });

    bunny_ = std::make_unique<ModelObject>();
    bunny_->Initialize();
    bunny_->SetPosition({ 0.0f, 2.0f, 0.0f });

    terrain_ = std::make_unique<ModelObject>();
    terrain_->Initialize();
    terrain_->SetScale(Vector3{ 2.0f, 2.0f, 2.0f });
    terrain_->SetPosition({ 0.0f, -1.0f, 0.0f });

    sphere_ = std::make_unique<SphereObject>();
    sphere_->Initialize();
    sphere_->SetPosition({ 2.0f, 0.0f, 0.0f });

    triangle_ = std::make_unique<TriangleObject>();
    triangle_->Initialize();
    triangle_->SetPosition({ -2.0f, 0.0f, 0.0f });

    particle_ = std::make_unique<ParticleObject>();
    particle_->Initialize();
    particle_->SetPosition({ 4.0f, 2.0f, 20.0f });

    sprite_ = std::make_unique<SpriteObject>();
    sprite_->Initialize();
    sprite_->SetSize(300.0f, 300.0f);
}

void App::Run() {
    // --- ImGui ---
#ifdef _USEIMGUI
    engine_->ImGuiBegin();
    debugCamera_->ImGuiControl();
    dirLight_->ImGuiControl("dirLight");
    dirLight2_->ImGuiControl("dirLight2");
    pointLight_->ImGuiControl("pointLight");
    pointLight2_->ImGuiControl("pointLight2");
    spotLight_->ImGuiControl("spotLight");
    skydome_->ImGuiControl("skydome");
    teapot_->ImGuiControl("teapot");
    bunny_->ImGuiControl("bunny");
    terrain_->ImGuiControl("terrain");
    sphere_->ImGuiControl("sphere");
    triangle_->ImGuiControl("triangle");
    particle_->ImGuiControl("particle");
    sprite_->ImGuiControl("sprite");
    engine_->ImGuiEnd();
#endif

    // --- 更新 ---
    if (input_->Trigger(DIK_TAB)) {
        static bool isDebug = false;
        isDebug = !isDebug;
        cameraMgr_->SetActiveCamera(isDebug ? "Debug" : "Main");
        debugCamera_->SetActive(isDebug);
    }

    if (cameraMgr_->GetActiveCamera() == mainCamera_.get()) {
        mainCamera_->Update();
    }else if (cameraMgr_->GetActiveCamera() == debugCamera_.get()) {
        debugCamera_->Update(input_.get());
    }

    input_->Update();
    cameraMgr_->Update();
    lightMgr_->Update();
    dirLight_->Update();
    dirLight2_->Update();
    pointLight_->Update();
    pointLight2_->Update();
    spotLight_->Update();
    skydome_->Update();
    teapot_->Update();
    bunny_->Update();
    terrain_->Update();
    sphere_->Update();
    triangle_->Update();
    particle_->Update();
    sprite_->Update();

    // --- 描画 ---
    engine_->BeginFrame();
    skydome_->Draw("skydome", "skydome");
    terrain_->Draw("terrain", "terrain");
    teapot_->Draw("teapot", "teapot");
    bunny_->Draw("bunny", "bunny");
    sphere_->Draw("monsterBall");
    triangle_->Draw("white");
    particle_->Draw("circle");
    sprite_->Draw("uvChecker");
    engine_->EndFrame();
}

void App::Finalize() {
	engine_->Finalize();
}

bool App::IsEnd() const {
    return !engine_->ProcessMessage();
}
