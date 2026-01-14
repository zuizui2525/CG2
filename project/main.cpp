#include "Zuizui.h"
#include "ModelObject.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    Zuizui* engine = Zuizui::GetInstance();
    engine->Initialize();

    auto teapot = std::make_unique<ModelObject>(engine->GetDevice(), "resources/obj/teapot/teapot.obj");
    engine->GetTextureManager()->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);

    while (engine->ProcessMessage()) {
        engine->GetCamera()->Update();
        Vector3 position{};
        if (engine->PressKey(DIK_SPACE)) {
            position.y += 1.0f;
        }
        teapot->SetPosition(position);
        teapot->Update(engine->GetCamera());

        engine->BeginFrame();
        engine->DrawModel(teapot.get(), "teapot");
        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
