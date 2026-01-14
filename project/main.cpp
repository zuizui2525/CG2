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

    auto sprite = std::make_unique<SpriteObject>(engine->GetDevice());
    engine->GetTextureManager()->LoadTexture("uvChecker", "resources/uvChecker.png");

    auto triangle = std::make_unique<TriangleObject>(engine->GetDevice());
    engine->GetTextureManager()->LoadTexture("white", "resources/white.png");

    while (engine->ProcessMessage()) {
        engine->Update();
        
        engine->BeginFrame();
        engine->DrawModel(teapot.get(), "teapot", Vector3{});
        engine->DrawSphere(sphere.get(), "monsterBall", Vector3{ 2.0f, 0.0f, 0.0f }, 0.5f);
        engine->DrawSprite(sprite.get(), "uvChecker", Vector2{}, 300.0f, 300.0f);
        engine->DrawTriangle(triangle.get(), "white", Vector3{ -2.0f, 0.0f, 0.0f });
        engine->EndFrame();
    }

    engine->Finalize();
    return 0;
}
