#include "Engine/Graphics/PostProcess/PostProcess.h"
#include "Engine/Graphics/PSO/Manager/PSOManager.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Zuizui.h"
#include "Engine/Base/WindowApp/WindowApp.h"
#include <cassert>

namespace {
    // マジックナンバー排除のためのクリアカラー定数
    const Vector4 kClearColorBlue = { 0.1f, 0.25f, 0.5f, 1.0f }; // 元々の背景色
    const Vector4 kClearColorRed = { 1.0f, 0.0f, 0.0f, 1.0f };
    const Vector4 kClearColorBlack = { 0.1f, 0.1f, 0.1f, 1.0f };

    // ルートパラメータインデックス
    constexpr UINT kRootParamIndexSRV = 0;

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

    // 現在のモードに応じた PSO を決定
    const char* psoName = "CopyImage";
    if (currentMode_ == PostEffectMode::Grayscale) {
        psoName = "Grayscale";
    } else if (currentMode_ == PostEffectMode::Sepia) {
        psoName = "Sepia";
    }

    commandList->SetPipelineState(psoManager->GetPSO(psoName));
    commandList->SetGraphicsRootSignature(psoManager->GetRootSignature(psoName));

    // ルートパラメータにレンダーテクスチャの SRV (GPUハンドルのディスクリプタテーブル) を設定
    commandList->SetGraphicsRootDescriptorTable(kRootParamIndexSRV, renderTexture_->GetSrvGpuHandle());

    // 頂点バッファなしで3頂点描画（全画面三角形）
    commandList->DrawInstanced(kVertexCount, kInstanceCount, kStartVertexLocation, kStartInstanceLocation);
}

namespace {
    // ヘルパー関数：モードに対応したクリアカラーを返す
    Vector4 GetClearColorForMode(PostEffectMode mode) {
        switch (mode) {
            case PostEffectMode::None:      return kClearColorBlue;
            case PostEffectMode::Red:       return kClearColorRed;
            case PostEffectMode::Black:
            case PostEffectMode::Grayscale:
            case PostEffectMode::Sepia:
            default:                        return kClearColorBlack;
        }
    }
}

void PostProcess::SetEffectMode(PostEffectMode mode) {
    if (currentMode_ == mode) return;

    // 背景色（クリアカラー）の変更チェック
    const Vector4 oldColor = GetClearColorForMode(currentMode_);
    const Vector4 newColor = GetClearColorForMode(mode);

    currentMode_ = mode;

    // 背景色が変わる場合のみ安全にRecreate
    if (oldColor.x != newColor.x || oldColor.y != newColor.y || oldColor.z != newColor.z) {
        renderTexture_->Recreate(newColor);
    }
}
