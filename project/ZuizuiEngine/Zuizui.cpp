#include "Zuizui.h"
#include "ModelObject.h"
#include "SphereObject.h"
#include "SpriteObject.h"
#include "TriangleObject.h"
#include "ParticleManager.h"
#include "ImguiManager.h"

Zuizui* Zuizui::instance = nullptr;

Zuizui* Zuizui::GetInstance() {
    if (!instance) instance = new Zuizui();
    return instance;
}

void Zuizui::Initialize() {
    window->Initialize(L"LE2B_02_イトウカズイ");
    window->Show();
    dxCommon->Initialize(window->GetHWND(), WindowApp::kClientWidth, WindowApp::kClientHeight);

    psoManager = std::make_unique<PSOManager>(dxCommon->GetDevice());
    psoManager->Initialize(dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());

    textureManager = std::make_unique<TextureManager>();
    textureManager->Initialize(dxCommon->GetDevice(), dxCommon->GetCommandList(), dxCommon->GetSrvHeap());

    input = std::make_unique<Input>();
    input->Initialize(window->GetInstance(), window->GetHWND());

    camera = std::make_unique<Camera>();
    camera->Initialize(dxCommon->GetDevice(), input.get());

    dirLight = std::make_unique<DirectionalLightObject>();
    dirLight->Initialize(dxCommon->GetDevice());

    audio->Initialize();

#ifdef _DEBUG
    imgui = std::make_unique<ImguiManager>();
    imgui->Initialize(window->GetHWND(), dxCommon->GetDevice(), dxCommon->GetBackBufferCount(), dxCommon->GetRtvFormat(), dxCommon->GetRtvHeap(), dxCommon->GetSrvHeap());
#endif
}

void Zuizui::BeginFrame() {
    dxCommon->FrameStart();
#ifdef _DEBUG
    imgui->Begin();
#endif
    dxCommon->BeginFrame();
    dxCommon->PreDraw();
}

void Zuizui::EndFrame() {
#ifdef _DEBUG
    imgui->End();
    dxCommon->DrawImGui();
#endif
    dxCommon->EndFrame();
    dxCommon->FrameEnd(60);
}

// 描画ラップ関数群
void Zuizui::DrawModel(ModelObject* model, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !model) return;
    model->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}

void Zuizui::DrawSphere(SphereObject* sphere, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !sphere) return;
    sphere->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}

void Zuizui::DrawParticle(ParticleManager* particle, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !particle) return;
    particle->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), true);
}

void Zuizui::Finalize() {
#ifdef _DEBUG
    imgui->Shutdown();
#endif
    // COMの終了処理
    CoUninitialize();
}
