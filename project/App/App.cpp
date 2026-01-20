#include "App.h"

void App::Initialize() {
    // システム
    engine_ = Zuizui::GetInstance();
    engine_->Initialize(L"LE2B_02_イトウカズイ");

    input_ = std::make_unique<Input>();
    camera_ = std::make_unique<Camera>();
    dirLight_ = std::make_unique<DirectionalLightObject>();
    texMgr_ = std::make_unique<TextureManager>();
    modelMgr_ = std::make_unique<ModelManager>();

    input_->Initialize(engine_->GetWindow()->GetInstance(), engine_->GetWindow()->GetHWND());
    camera_->Initialize(engine_->GetDevice(), input_.get());
    dirLight_->Initialize(engine_->GetDevice());
    texMgr_->Initialize(engine_->GetDevice(), engine_->GetDxCommon()->GetCommandList(), engine_->GetDxCommon()->GetSrvHeap());
    modelMgr_->Initialize(engine_->GetDevice(), texMgr_.get());

    // 静的リソース（シングルトン的な役割）への登録
    EngineResource::SetEngine(engine_);
    TextureResource::SetTextureManager(texMgr_.get());
    CameraResource::SetCamera(camera_.get());
    LightResource::SetLight(dirLight_.get());
    ModelResource::SetModelManager(modelMgr_.get());

    // テクスチャ
    texMgr_->LoadTexture("white", "resources/white.png");
    texMgr_->LoadTexture("monsterBall", "resources/monsterball.png");
    texMgr_->LoadTexture("circle", "resources/circle.png");
    texMgr_->LoadTexture("uvChecker", "resources/uvChecker.png");

    // モデル
    modelMgr_->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
    modelMgr_->LoadModel("bunny", "resources/obj/bunny/bunny.obj");

    // ゲームオブジェクト
    teapot_ = std::make_unique<ModelObject>();
    teapot_->Initialize();
    teapot_->SetPosition({ 0.0f, 0.0f, 0.0f });

    bunny_ = std::make_unique<ModelObject>();
    bunny_->Initialize();
    bunny_->SetPosition({ 0.0f, 2.0f, 0.0f });

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
    // --- 更新 ---
    input_->Update();
    camera_->Update();
    dirLight_->Update();
    teapot_->Update();
    bunny_->Update();
    sphere_->Update();
    triangle_->Update();
    particle_->Update();
    sprite_->Update();

    // --- 描画 ---
    engine_->BeginFrame();
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
