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

        engine->BeginFrame();
        engine->DrawModel(Vector3{}, teapot.get(), "teapot", false);
        engine->DrawSphere(Vector3{}, sphere.get(), "monsterBall");
        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
