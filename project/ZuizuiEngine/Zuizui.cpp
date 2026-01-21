#include "Zuizui.h"
#include "ImguiManager.h"

Zuizui* Zuizui::instance = nullptr;

Zuizui* Zuizui::GetInstance() {
    if (!instance) instance = new Zuizui();
    return instance;
}

void Zuizui::Initialize(const wchar_t* title, const int32_t width, const int32_t height) {
    window = std::make_unique<WindowApp>();
    window->Initialize(title, width, height);
    window->Show();

    dxCommon = std::make_unique<DxCommon>();
    dxCommon->Initialize(window->GetHWND(), width, height);

    psoManager = std::make_unique<PSOManager>(dxCommon->GetDevice());
    psoManager->Initialize(dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());
#ifdef _DEBUG
    imGui = std::make_unique<ImguiManager>();
    imGui->Initialize(window->GetHWND(), dxCommon->GetDevice(), dxCommon->GetBackBufferCount(), dxCommon->GetRtvFormat(), dxCommon->GetRtvHeap(), dxCommon->GetSrvHeap());
#endif
}

void Zuizui::Finalize() {
#ifdef _DEBUG
    imGui->Shutdown();
#endif
    // COMの終了処理
    CoUninitialize();

    // 明示的に開放
    delete instance;
    instance = nullptr;
}

void Zuizui::ImGuiBegin() {
#ifdef _DEBUG
    imGui->Begin();
#endif
}

void Zuizui::ImGuiEnd() {
#ifdef _DEBUG
    imGui->End();
#endif
}

void Zuizui::BeginFrame() {
    dxCommon->FrameStart();
    dxCommon->BeginFrame();
    dxCommon->PreDraw();
}

void Zuizui::EndFrame() {
#ifdef _DEBUG
    dxCommon->DrawImGui();
#endif
    dxCommon->EndFrame();
    dxCommon->FrameEnd(60);
}

