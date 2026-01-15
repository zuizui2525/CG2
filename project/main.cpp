#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "Input.h"

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

    auto teapot = std::make_unique<ModelObject>();
    teapot->Initialize(engine, camera.get(), dirLight.get(), "resources/obj/teapot/teapot.obj");
    teapot->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);
    teapot->SetPosition(Vector3{ 0.0f, 0.0f, 0.0f });

    auto sphere = std::make_unique<SphereObject>();
    sphere->Initialize(engine, camera.get(), dirLight.get());
    sphere->LoadTexture("monsterBall", "resources/monsterball.png");
    sphere->SetPosition(Vector3{ 2.0f, 0.0f, 0.0f });

    auto triangle = std::make_unique<TriangleObject>();
    triangle->Initialize(engine, camera.get(), dirLight.get());
    triangle->LoadTexture("white", "resources/white.png");
    triangle->SetPosition(Vector3{ -2.0f, 0.0f, 0.0f });

    while (engine->ProcessMessage()) {
        input->Update();
        camera->Update();
        dirLight->Update();

        teapot->Update();
        sphere->Update();
        triangle->Update();

        engine->BeginFrame();
       
        teapot->Draw("teapot");
        sphere->Draw("monsterBall");
        triangle->Draw("white");

        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
