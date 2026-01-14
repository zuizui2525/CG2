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
    window = std::make_unique<WindowApp>();
    window->Initialize(L"LE2B_02_イトウカズイ");
    window->Show();

    dxCommon = std::make_unique<DxCommon>();
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

    audio = std::make_unique<Audio>();
    audio->Initialize();

#ifdef _DEBUG
    imgui = std::make_unique<ImguiManager>();
    imgui->Initialize(window->GetHWND(), dxCommon->GetDevice(), dxCommon->GetBackBufferCount(), dxCommon->GetRtvFormat(), dxCommon->GetRtvHeap(), dxCommon->GetSrvHeap());
#endif
}

void Zuizui::Update() {
    input->Update();
    camera->Update();
    dirLight->Update();
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
void Zuizui::DrawModel(Vector3 position, ModelObject* model, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !model) return;
    model->SetPosition(position);
    model->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}

void Zuizui::DrawSphere(Vector3 position, SphereObject* sphere, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !sphere) return;
    sphere->SetPosition(position);
    sphere->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}

void Zuizui::DrawSprite(Vector3 position, SpriteObject* sprite, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !sprite) return;
    sprite->SetPosition(position);
    sprite->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}

void Zuizui::DrawTriangle(Vector3 position, TriangleObject* triangle, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !triangle) return;
    triangle->SetPosition(position);
    triangle->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), true);
}

void Zuizui::DrawParticle(Vector3 position, ParticleManager* particle, const std::string& textureKey, bool drawFlag) {
    if (!drawFlag || !particle) return;
    particle->SetPosition(position);
    particle->Draw(dxCommon->GetCommandList(), textureManager->GetGpuHandle(textureKey), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), true);
}

bool Zuizui::TriggerKey(BYTE key) const {
    return input->Trigger(key);
}

bool Zuizui::PressKey(BYTE key) const {
    return input->Press(key);
}

bool Zuizui::ReleaseKey(BYTE key) const {
    return input->Release(key);
}

void Zuizui::Finalize() {
#ifdef _DEBUG
    imgui->Shutdown();
#endif
    // COMの終了処理
    CoUninitialize();

    // 明示的に開放
    delete instance;
    instance = nullptr;
}
