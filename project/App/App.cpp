#include "App.h"

void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
    engine_->Initialize(L"LE2B_02_イトウカズイ");
    EngineResource::SetEngine(engine_);

    input_ = std::make_unique<Input>();
    input_->Initialize(engine_->GetWindow()->GetInstance(), engine_->GetWindow()->GetHWND());

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetDevice(), input_.get());
    CameraResource::SetCamera(camera_.get());

    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize(engine_->GetDevice());
    LightResource::SetLight(dirLight_.get());

    lightManager_ = std::make_unique<LightManager>();
    lightManager_->Initialize();
    LightResource::SetLightManager(lightManager_.get());

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
    pointLight_ = std::make_unique<PointLightObject>();
    pointLight_->Initialize();
    lightManager_->AddPointLight(pointLight_.get());

    pointLight2_ = std::make_unique<PointLightObject>();
    pointLight2_->Initialize();
    pointLight2_->SetPosition({ -5.0f, 2.0f, 0.0f });
    lightManager_->AddPointLight(pointLight2_.get());

    spotLight_ = std::make_unique<SpotLightObject>();
    spotLight_->Initialize();
    lightManager_->AddSpotLight(spotLight_.get());

    skydome_ = std::make_unique<ModelObject>();
    skydome_->Initialize();
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
    camera_->ImGuiControl("camera");
    dirLight_->ImGuiControl("dirLight");
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
    input_->Update();
    camera_->Update();
    dirLight_->Update();
    lightManager_->Update();
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
