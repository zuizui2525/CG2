#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "Input.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ModelObject.h"
#include "SphereObject.h"
#include "SpriteObject.h"
#include "TriangleObject.h"
#include "ParticleObject.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    Zuizui* engine = Zuizui::GetInstance();
    engine->Initialize(L"LE2B_02_イトウカズイ");
    
    std::unique_ptr<Input> input = std::make_unique<Input>();
    input->Initialize(engine->GetWindow()->GetInstance(), engine->GetWindow()->GetHWND());

    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    camera->Initialize(engine->GetDevice(), input.get());

    std::unique_ptr<DirectionalLightObject> dirLight = std::make_unique<DirectionalLightObject>();
    dirLight->Initialize(engine->GetDevice());

    std::unique_ptr<TextureManager> textureMgr = std::make_unique<TextureManager>();
    textureMgr->Initialize(engine->GetDevice(), engine->GetDxCommon()->GetCommandList(), engine->GetDxCommon()->GetSrvHeap());
    
    std::unique_ptr<ModelManager> modelMgr = std::make_unique<ModelManager>();
    modelMgr->Initialize(engine->GetDevice(), textureMgr.get());

    auto teapot = std::make_unique<ModelObject>();
    teapot->Initialize(engine, camera.get(), dirLight.get(), textureMgr.get(), modelMgr.get());
    modelMgr->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
    teapot->SetPosition(Vector3{ 0.0f, 0.0f, 0.0f });

    auto bunny = std::make_unique<ModelObject>();
    bunny->Initialize(engine, camera.get(), dirLight.get(), textureMgr.get(), modelMgr.get());
    modelMgr->LoadModel("bunny", "resources/obj/bunny/bunny.obj");
    bunny->SetPosition(Vector3{ 0.0f, 2.0f, 0.0f });

    auto sphere = std::make_unique<SphereObject>();
    sphere->Initialize(engine, camera.get(), dirLight.get(), textureMgr.get());
    textureMgr->LoadTexture("monsterBall", "resources/monsterball.png");
    sphere->SetPosition(Vector3{ 2.0f, 0.0f, 0.0f });

    auto triangle = std::make_unique<TriangleObject>();
    triangle->Initialize(engine, camera.get(), dirLight.get(), textureMgr.get());
    textureMgr->LoadTexture("white", "resources/white.png");
    triangle->SetPosition(Vector3{ -2.0f, 0.0f, 0.0f });

    auto particle = std::make_unique<ParticleObject>();
    particle->Initialize(engine, camera.get(), dirLight.get(), textureMgr.get());
    textureMgr->LoadTexture("circle", "resources/circle.png");
    particle->SetPosition(Vector3{ 4.0f, 2.0f, 20.0f });

    auto sprite = std::make_unique<SpriteObject>();
    sprite->Initialize(engine, camera.get(), dirLight.get(), textureMgr.get());
    textureMgr->LoadTexture("uvChecker", "resources/uvChecker.png");
    sprite->SetSize(300.0f, 300.0f);

    while (engine->ProcessMessage()) {
        input->Update();
        camera->Update();
        dirLight->Update();

        teapot->Update();
        bunny->Update();
        sphere->Update();
        triangle->Update();
        particle->Update();
        sprite->Update();

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
