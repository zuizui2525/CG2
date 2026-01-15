#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    Zuizui* engine = Zuizui::GetInstance();
    engine->Initialize(L"LE2B_02_イトウカズイ");
    
    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    camera->Initialize(engine->GetDevice(), input.get());

   std::unique_ptr<DirectionalLightObject> dirLight = std::make_unique<DirectionalLightObject>();
    dirLight->Initialize(engine->GetDevice());

    auto teapot = std::make_unique<ModelObject>();
    teapot->Initialize(engine, camera.get(), dirLight.get(), "resources/obj/teapot/teapot.obj");
    engine->GetTextureManager()->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);

    auto sphere = std::make_unique<SphereObject>();
    sphere->Initialize(engine->GetDevice());
    engine->GetTextureManager()->LoadTexture("monsterBall", "resources/monsterball.png");

    auto sprite = std::make_unique<SpriteObject>();
    sprite->Initialize(engine->GetDevice());
    engine->GetTextureManager()->LoadTexture("uvChecker", "resources/uvChecker.png");

    auto triangle = std::make_unique<TriangleObject>();
    triangle->Initialize(engine->GetDevice());
    engine->GetTextureManager()->LoadTexture("white", "resources/white.png");

    auto particle = std::make_unique<ParticleManager>();
    particle->Initialize(engine->GetDxCommon());
    engine->GetTextureManager()->LoadTexture("circle", "resources/circle.png");

    while (engine->ProcessMessage()) {
        
        engine->BeginFrame();
       
        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
