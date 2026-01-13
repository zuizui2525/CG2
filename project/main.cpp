#include "Zuizui.h"
#include "ModelObject.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    Zuizui* engine = Zuizui::GetInstance();
    engine->Initialize();

    auto teapot = std::make_unique<ModelObject>(engine->GetDevice(), "resources/obj/teapot/teapot.obj", Vector3{ 1,0,0 });
    engine->GetTextureManager()->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);

    while (engine->ProcessMessage()) {
        engine->GetCamera()->Update();
        teapot->Update(engine->GetCamera());

        engine->BeginFrame();
        engine->DrawModel(teapot.get(), "teapot");
        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
