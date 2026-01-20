#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "Input.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "BaseObject.h"
#include "ModelObject.h"
#include "SphereObject.h"
#include "SpriteObject.h"
#include "TriangleObject.h"
#include "ParticleObject.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // --- 1. システム初期化 ---
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    Zuizui* engine = Zuizui::GetInstance();
    engine->Initialize(L"LE2B_02_イトウカズイ");

    // --- 2. 各種マネージャ・共有リソースの生成 ---
    auto input = std::make_unique<Input>();
    auto camera = std::make_unique<Camera>();
    auto dirLight = std::make_unique<DirectionalLightObject>();
    auto texMgr = std::make_unique<TextureManager>();
    auto modelMgr = std::make_unique<ModelManager>();

    input->Initialize(engine->GetWindow()->GetInstance(), engine->GetWindow()->GetHWND());
    camera->Initialize(engine->GetDevice(), input.get());
    dirLight->Initialize(engine->GetDevice());
    texMgr->Initialize(engine->GetDevice(), engine->GetDxCommon()->GetCommandList(), engine->GetDxCommon()->GetSrvHeap());
    modelMgr->Initialize(engine->GetDevice(), texMgr.get());

    // --- 3. BaseObject系へのリソース登録 (一箇所に集約) ---
    EngineResource::SetEngine(engine);
    TextureResource::SetTextureManager(texMgr.get());
    CameraResource::SetCamera(camera.get());
    LightResource::SetLight(dirLight.get());
    ModelResource::SetModelManager(modelMgr.get());

    // --- 4. ゲームオブジェクトの生成と設定 ---
    // テクスチャ読み込み
    texMgr->LoadTexture("white", "resources/white.png");
    texMgr->LoadTexture("monsterBall", "resources/monsterball.png");
    texMgr->LoadTexture("circle", "resources/circle.png");
    texMgr->LoadTexture("uvChecker", "resources/uvChecker.png");

    // モデル読み込み
    modelMgr->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
    modelMgr->LoadModel("bunny", "resources/obj/bunny/bunny.obj");

    // 各オブジェクト初期化 (引数が不要になったのでスッキリ！)
    auto teapot = std::make_unique<ModelObject>();
    teapot->Initialize();
    teapot->SetPosition({ 0.0f, 0.0f, 0.0f });

    auto bunny = std::make_unique<ModelObject>();
    bunny->Initialize();
    bunny->SetPosition({ 0.0f, 2.0f, 0.0f });

    auto sphere = std::make_unique<SphereObject>();
    sphere->Initialize();
    sphere->SetPosition({ 2.0f, 0.0f, 0.0f });

    auto triangle = std::make_unique<TriangleObject>();
    triangle->Initialize();
    triangle->SetPosition({ -2.0f, 0.0f, 0.0f });

    auto particle = std::make_unique<ParticleObject>();
    particle->Initialize();
    particle->SetPosition({ 4.0f, 2.0f, 20.0f });

    auto sprite = std::make_unique<SpriteObject>();
    sprite->Initialize();
    sprite->SetSize(300.0f, 300.0f);

    // --- 5. メインループ ---
    while (engine->ProcessMessage()) {
        // 更新
        input->Update();
        camera->Update();
        dirLight->Update();

        teapot->Update();
        bunny->Update();
        sphere->Update();
        triangle->Update();
        particle->Update();
        sprite->Update();

        // 描画
        engine->BeginFrame();

        teapot->Draw("teapot", "teapot");
        bunny->Draw("bunny", "bunny");
        sphere->Draw("monsterBall");
        triangle->Draw("white");
        particle->Draw("circle");
        sprite->Draw("uvChecker");

        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
