#include "Engine/Graphics/PostProcess/PostProcess.h"
#include "Engine/Graphics/PSO/Manager/PSOManager.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Zuizui.h"
#include "Engine/Base/WindowApp/WindowApp.h"
#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/DxUtils.h"
#include <cassert>
#include <format>

#ifdef _USEIMGUI
#include <imgui.h>
#endif

namespace {
    // マジックナンバー排除のためのクリアカラー定数
    const Vector4 kClearColorBlue = { 0.1f, 0.25f, 0.5f, 1.0f }; // 元々の背景色
    const Vector4 kClearColorRed = { 1.0f, 0.0f, 0.0f, 1.0f };
    const Vector4 kClearColorBlack = { 0.1f, 0.1f, 0.1f, 1.0f };

    // Vignetteのデフォルト値
    const float kDefaultVignetteScale = 16.0f;
    const float kDefaultVignetteExponent = 0.8f;

    // ルートパラメータインデックス
    constexpr UINT kRootParamIndexSRV = 0;
    constexpr UINT kRootParamIndexPostProcessCBV = 1;

    // 描画用の頂点数およびインスタンス数
    constexpr UINT kVertexCount = 3;
    constexpr UINT kInstanceCount = 1;
    constexpr UINT kStartVertexLocation = 0;
    constexpr UINT kStartInstanceLocation = 0;
}

void PostProcess::Initialize() {
    Zuizui* engine = EngineResource::GetEngine();
    assert(engine != nullptr);

    renderTexture_ = std::make_unique<RenderTexture>();

    // 初期化時のクリアカラー（デフォルトは青）
    const Vector4 kClearColor = kClearColorBlue;

    // RTVは拡張したヒープのインデックス2を使用
    constexpr UINT kRtvIndex = 2;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = engine->GetDxCommon()->GetRtvHandle(kRtvIndex);

    // SRVヒープの末尾（インデックス127）を静的に割り当て
    ID3D12DescriptorHeap* srvHeap = engine->GetDxCommon()->GetSrvHeap();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvHeap->GetGPUDescriptorHandleForHeapStart();
    UINT srvDescriptorSize = engine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    constexpr UINT kReservedSrvIndex = 127;
    srvHandleCPU.ptr += kReservedSrvIndex * srvDescriptorSize;
    srvHandleGPU.ptr += kReservedSrvIndex * srvDescriptorSize;

    renderTexture_->Initialize(
        engine->GetDevice(),
        WindowApp::kClientWidth,
        WindowApp::kClientHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        kClearColor,
        rtvHandle,
        srvHandleCPU,
        srvHandleGPU
    );

    // 統合ポストプロセス用定数バッファ作成
    postProcessResource_ = DxUtils::CreateBufferResource(engine->GetDevice(), sizeof(PostProcessParams));
    assert(postProcessResource_ != nullptr && "Failed to create postProcessResource_");

    HRESULT hr = postProcessResource_->Map(0, nullptr, reinterpret_cast<void**>(&postProcessData_));
    assert(SUCCEEDED(hr) && postProcessData_ != nullptr && "Failed to map postProcessResource_");

    // デフォルト値設定 (全てのエフェクトは最初はOFF)
    postProcessData_->enableGrayscale = 0;
    postProcessData_->enableSepia = 0;
    postProcessData_->enableVignette = 0;
    postProcessData_->vignetteScale = kDefaultVignetteScale;
    postProcessData_->vignetteExponent = kDefaultVignetteExponent;
}

void PostProcess::PreDraw() {
    Zuizui* engine = EngineResource::GetEngine();
    assert(engine != nullptr);
    assert(renderTexture_ != nullptr);

    ID3D12GraphicsCommandList* commandList = engine->GetDxCommon()->GetCommandList();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = engine->GetDxCommon()->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();

    renderTexture_->PreDraw(commandList, dsvHandle);
}

void PostProcess::PostDraw() {
    Zuizui* engine = EngineResource::GetEngine();
    assert(engine != nullptr);
    assert(renderTexture_ != nullptr);

    ID3D12GraphicsCommandList* commandList = engine->GetDxCommon()->GetCommandList();
    renderTexture_->PostDraw(commandList);
}

void PostProcess::Draw() {
    Zuizui* engine = EngineResource::GetEngine();
    assert(engine != nullptr);
    assert(renderTexture_ != nullptr);

    ID3D12GraphicsCommandList* commandList = engine->GetDxCommon()->GetCommandList();
    PSOManager* psoManager = engine->GetPSOManager();
    assert(psoManager != nullptr);

    // スワップチェーンの現在のバックバッファRTVを取得
    UINT backBufferIndex = engine->GetDxCommon()->GetBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE swapchainRtv = engine->GetDxCommon()->GetRtvHandle(backBufferIndex);

    // レンダーターゲットをスワップチェーンのバックバッファに切り替え
    commandList->OMSetRenderTargets(1, &swapchainRtv, FALSE, nullptr);

    // 統合された PostProcess PSO を使用
    const char* psoName = "PostProcess";

    commandList->SetPipelineState(psoManager->GetPSO(psoName));
    commandList->SetGraphicsRootSignature(psoManager->GetRootSignature(psoName));

    // ルートパラメータにレンダーテクスチャの SRV (GPUハンドルのディスクリプタテーブル) を設定
    commandList->SetGraphicsRootDescriptorTable(kRootParamIndexSRV, renderTexture_->GetSrvGpuHandle());

    // 定数バッファをセット (RootParameter Index 1)
    commandList->SetGraphicsRootConstantBufferView(kRootParamIndexPostProcessCBV, postProcessResource_->GetGPUVirtualAddress());

    // 頂点バッファなしで3頂点描画（全画面三角形）
    commandList->DrawInstanced(kVertexCount, kInstanceCount, kStartVertexLocation, kStartInstanceLocation);
}

void PostProcess::SetClearColorMode(PostClearColorMode mode) {
    if (clearColorMode_ == mode) return;

    Vector4 oldColor = kClearColorBlue;
    switch (clearColorMode_) {
        case PostClearColorMode::Blue:  oldColor = kClearColorBlue; break;
        case PostClearColorMode::Red:   oldColor = kClearColorRed; break;
        case PostClearColorMode::Black: oldColor = kClearColorBlack; break;
    }

    Vector4 newColor = kClearColorBlue;
    switch (mode) {
        case PostClearColorMode::Blue:  newColor = kClearColorBlue; break;
        case PostClearColorMode::Red:   newColor = kClearColorRed; break;
        case PostClearColorMode::Black: newColor = kClearColorBlack; break;
    }

    clearColorMode_ = mode;

    if (oldColor.x != newColor.x || oldColor.y != newColor.y || oldColor.z != newColor.z) {
        renderTexture_->Recreate(newColor);
    }
}

void PostProcess::SetEffectMode(PostEffectMode mode) {
    if (currentMode_ == mode) return;
    currentMode_ = mode;

    // 互換モードに基づいてクリアカラーとエフェクトを切り替える
    switch (mode) {
        case PostEffectMode::None:
            SetClearColorMode(PostClearColorMode::Blue);
            SetGrayscaleActive(false);
            SetSepiaActive(false);
            SetVignetteActive(false);
            break;
        case PostEffectMode::Red:
            SetClearColorMode(PostClearColorMode::Red);
            SetGrayscaleActive(false);
            SetSepiaActive(false);
            SetVignetteActive(false);
            break;
        case PostEffectMode::Black:
            SetClearColorMode(PostClearColorMode::Black);
            SetGrayscaleActive(false);
            SetSepiaActive(false);
            SetVignetteActive(false);
            break;
        case PostEffectMode::Grayscale:
            SetClearColorMode(PostClearColorMode::Black);
            SetGrayscaleActive(true);
            SetSepiaActive(false);
            SetVignetteActive(false);
            break;
        case PostEffectMode::Sepia:
            SetClearColorMode(PostClearColorMode::Black);
            SetGrayscaleActive(false);
            SetSepiaActive(true);
            SetVignetteActive(false);
            break;
        case PostEffectMode::Vignette:
            SetClearColorMode(PostClearColorMode::Black);
            SetGrayscaleActive(false);
            SetSepiaActive(false);
            SetVignetteActive(true);
            break;
    }

    // 日本語ログ出力
    std::wstring modeName = L"不明";
    switch (mode) {
        case PostEffectMode::None:      modeName = L"なし (通常ブルー背景)"; break;
        case PostEffectMode::Red:       modeName = L"デバッグレッド"; break;
        case PostEffectMode::Black:     modeName = L"デバッグブラック"; break;
        case PostEffectMode::Grayscale: modeName = L"グレースケール"; break;
        case PostEffectMode::Sepia:     modeName = L"セピア"; break;
        case PostEffectMode::Vignette:  modeName = L"ビネット"; break;
    }

    Log::Write(std::format(L" ├─ 【ポストエフェクト変更】 エフェクトモードが「{}」に変更されました。", modeName));
}

void PostProcess::ImGuiControl() {
#ifdef _USEIMGUI
    ImGui::Begin("Settings");
    ImGui::Checkbox("Vignette Settings", &isVignetteWindowOpen_);
    ImGui::End();

    if (isVignetteWindowOpen_) {
        if (ImGui::Begin("Vignette Control", &isVignetteWindowOpen_)) {
            ImGui::DragFloat("Scale", &postProcessData_->vignetteScale, 0.1f, 0.0f, 100.0f, "%.1f");
            ImGui::DragFloat("Exponent", &postProcessData_->vignetteExponent, 0.01f, 0.0f, 10.0f, "%.2f");
        }
        ImGui::End();
    }
#endif
}
