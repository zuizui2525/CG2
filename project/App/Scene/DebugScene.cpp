#include "App/Scene/DebugScene.h"

void DebugScene::Initialize() {
    // 1. 各マネージャのポインタを取得して保持する
    texMgr_ = TextureResource::GetTextureManager();
    modelMgr_ = ModelResource::GetModelManager();
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // 2. リソースの読み込み
    texMgr_->LoadTexture("white", "resources/white.png");
    texMgr_->LoadTexture("monsterBall", "resources/monsterBall.png");
    texMgr_->LoadTexture("circle", "resources/circle.png");
    texMgr_->LoadTexture("uvChecker", "resources/uvChecker.png");

    modelMgr_->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
    modelMgr_->LoadModel("bunny", "resources/obj/bunny/bunny.obj");
    modelMgr_->LoadModel("terrain", "resources/obj/terrain/terrain.obj");
    modelMgr_->LoadModel("skydome", "resources/obj/skydome/skydome.obj");

    // 3. カメラの生成と設定
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);

    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera("Debug", debugCamera_);
    cameraMgr_->SetActiveCamera("Main");

    // 4. ライトの生成
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

    // 5. オブジェクトの生成
    skydome_ = std::make_unique<ModelObject>();
    skydome_->Initialize();
    skydome_->SetLightingMode(0);

    teapot_ = std::make_unique<ModelObject>();
    teapot_->Initialize();

    bunny_ = std::make_unique<ModelObject>();
    bunny_->Initialize();
    bunny_->SetPosition({ 0.0f, 2.0f, 0.0f });

    terrain_ = std::make_unique<ModelObject>();
    terrain_->Initialize();
    terrain_->SetScale({ 2.0f, 2.0f, 2.0f });
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
    sprite_->SetPosition({ 100.0f, 100.0f });
    sprite_->SetSize(300.0f, 300.0f);
}

void DebugScene::ImGuiControl() {
#ifdef _USEIMGUI
    // シーン内のオブジェクトのデバッグ表示
    cameraMgr_->ImGuiControl();
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
#endif
}

void DebugScene::Update() {
    // モード切り替え（TABキー）
    if (input_->Trigger(DIK_TAB)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? "Main" : "Debug");
    }

    // オブジェクトの更新
    lightMgr_->Update();
    dirLight_->Update();
    dirLight2_->Update();
    pointLight_->Update();
    pointLight2_->Update();
    spotLight_->Update();

    skydome_->Update();
    terrain_->Update();
    teapot_->Update();
    bunny_->Update();
    sphere_->Update();
    triangle_->Update();
    particle_->Update();
    sprite_->Update();

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
    skydome_->Draw("skydome", "skydome");
    terrain_->Draw("terrain", "terrain");
    teapot_->Draw("teapot", "teapot");
    bunny_->Draw("bunny", "bunny");
    sphere_->Draw("monsterBall");
    triangle_->Draw("white");
    particle_->Draw("circle");
    sprite_->Draw("uvChecker");
}
