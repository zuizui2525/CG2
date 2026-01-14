#include "Zuizui.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    Zuizui* engine = Zuizui::GetInstance();
    engine->Initialize();

    auto teapot = std::make_unique<ModelObject>(engine->GetDevice(), "resources/obj/teapot/teapot.obj");
    engine->GetTextureManager()->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);

    auto sphere = std::make_unique<SphereObject>(engine->GetDevice());
    engine->GetTextureManager()->LoadTexture("monsterBall", "resources/monsterball.png");

    while (engine->ProcessMessage()) {
        engine->Update();
        teapot->Update(engine->GetCamera());
        sphere->Update(engine->GetCamera());

        engine->BeginFrame();
        engine->DrawModel(teapot.get(), "teapot", Vector3{});
        engine->DrawSphere(sphere.get(), "monsterBall", Vector3{ 2.0f, 0.0f, 0.0f }, 1.0f);
        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
