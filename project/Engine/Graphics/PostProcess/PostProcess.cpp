#include "Engine/Graphics/PostProcess/PostProcess.h"
#include "Engine/Graphics/PostProcess/CopyImagePass.h"
#include "Engine/Graphics/PostProcess/GrayscalePass.h"
#include "Engine/Graphics/PostProcess/SepiaPass.h"
#include "Engine/Graphics/PostProcess/VignettePass.h"
#include "Engine/Graphics/PostProcess/BoxFilterPass.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Zuizui.h"
#include "Engine/Base/WindowApp/WindowApp.h"
#include "Engine/Base/Log/Log.h"
#include <cassert>
#include <format>

namespace {
    // マジックナンバー排除のためのクリアカラー定数
    const Vector4 kClearColorBlue = { 0.1f, 0.25f, 0.5f, 1.0f }; // 元々の背景色
    const Vector4 kClearColorRed = { 1.0f, 0.0f, 0.0f, 1.0f };
    const Vector4 kClearColorBlack = { 0.1f, 0.1f, 0.1f, 1.0f };

    // 各パスのインデックス定義
    constexpr size_t kPassIndexCopy = 0;
    constexpr size_t kPassIndexGrayscale = 1;
    constexpr size_t kPassIndexSepia = 2;
    constexpr size_t kPassIndexVignette = 3;
    constexpr size_t kPassIndexBoxFilter = 4;
}

void PostProcess::Initialize() {
    Zuizui* engine = EngineResource::GetEngine();
    assert(engine != nullptr);

    renderTexture_ = std::make_unique<RenderTexture>();
    renderTextureTemp_ = std::make_unique<RenderTexture>();

    // 初期化時のクリアカラー（デフォルトは青）
    const Vector4 kClearColor = kClearColorBlue;

    // RTVは拡張したヒープのインデックス2および3を使用
    constexpr UINT kRtvIndex = 2;
    constexpr UINT kRtvIndexTemp = 3;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = engine->GetDxCommon()->GetRtvHandle(kRtvIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleTemp = engine->GetDxCommon()->GetRtvHandle(kRtvIndexTemp);

    // SRVヒープの末尾（インデックス127および126）を静的に割り当て
    ID3D12DescriptorHeap* srvHeap = engine->GetDxCommon()->GetSrvHeap();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPUTemp = srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPUTemp = srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    UINT srvDescriptorSize = engine->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    constexpr UINT kReservedSrvIndex = 127;
    constexpr UINT kReservedSrvIndexTemp = 126;
    srvHandleCPU.ptr += kReservedSrvIndex * srvDescriptorSize;
    srvHandleGPU.ptr += kReservedSrvIndex * srvDescriptorSize;
    srvHandleCPUTemp.ptr += kReservedSrvIndexTemp * srvDescriptorSize;
    srvHandleGPUTemp.ptr += kReservedSrvIndexTemp * srvDescriptorSize;

    // メインのレンダーテクスチャ初期化
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

    // ピンポン用中間テクスチャ初期化
    renderTextureTemp_->Initialize(
        engine->GetDevice(),
        WindowApp::kClientWidth,
        WindowApp::kClientHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        kClearColor,
        rtvHandleTemp,
        srvHandleCPUTemp,
        srvHandleGPUTemp
    );

    // 各個別パスの登録と初期化
    passes_.push_back(std::make_unique<CopyImagePass>());  // 0: Copy
    passes_.push_back(std::make_unique<GrayscalePass>());  // 1: Grayscale
    passes_.push_back(std::make_unique<SepiaPass>());      // 2: Sepia
    passes_.push_back(std::make_unique<VignettePass>());   // 3: Vignette
    passes_.push_back(std::make_unique<BoxFilterPass>());  // 4: BoxFilter

    for (auto& pass : passes_) {
        pass->Initialize(engine->GetDevice());
    }
}

void PostProcess::PreDraw() {
    Zuizui* engine = EngineResource::GetEngine();
    assert(engine != nullptr);
    assert(renderTexture_ != nullptr);

    ID3D12GraphicsCommandList* commandList = engine->GetDxCommon()->GetCommandList();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = engine->GetDxCommon()->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();

    // メインのレンダーテクスチャを描画ターゲットに設定してクリア
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
    assert(renderTextureTemp_ != nullptr);

    ID3D12GraphicsCommandList* commandList = engine->GetDxCommon()->GetCommandList();

    // スワップチェーンの現在のバックバッファRTVを取得
    UINT backBufferIndex = engine->GetDxCommon()->GetBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE swapchainRtv = engine->GetDxCommon()->GetRtvHandle(backBufferIndex);

    // アクティブなパスを収集（CopyImagePassは除く）
    std::vector<IPostProcessPass*> activePasses;
    for (size_t i = 1; i < passes_.size(); ++i) {
        if (passes_[i]->IsActive()) {
            activePasses.push_back(passes_[i].get());
        }
    }

    // レンダーターゲット遷移用の深度ステンシルハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = engine->GetDxCommon()->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();

    if (activePasses.empty()) {
        // アクティブなエフェクトパスがない場合は、メインテクスチャからスワップチェーンへ等倍コピーを描画
        commandList->OMSetRenderTargets(1, &swapchainRtv, FALSE, nullptr);
        passes_[kPassIndexCopy]->Draw(commandList, renderTexture_->GetSrvGpuHandle());
    } else {
        // ピンポンバッファのポインタ切り替えによるチェイン描画
        RenderTexture* currentInput = renderTexture_.get();
        RenderTexture* currentOutput = renderTextureTemp_.get();

        for (size_t i = 0; i < activePasses.size(); ++i) {
            bool isLast = (i == activePasses.size() - 1);

            if (isLast) {
                // 最後のパスは結果を直接スワップチェーン（画面）に描画
                commandList->OMSetRenderTargets(1, &swapchainRtv, FALSE, nullptr);
                activePasses[i]->Draw(commandList, currentInput->GetSrvGpuHandle());
            } else {
                // 中間パスは、一時バッファに出力
                currentOutput->PreDraw(commandList, dsvHandle);
                
                activePasses[i]->Draw(commandList, currentInput->GetSrvGpuHandle());
                
                currentOutput->PostDraw(commandList);

                // ピンポン切り替え
                currentInput = currentOutput;
                currentOutput = (currentInput == renderTextureTemp_.get()) ? renderTexture_.get() : renderTextureTemp_.get();
            }
        }
    }
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
        renderTextureTemp_->Recreate(newColor); // 中間テクスチャも再生成して色を一致させる
    }
}

void PostProcess::ImGuiControl() {
#ifdef _USEIMGUI
    ImGui::Begin("PostEffect");
    
    if (ImGui::TreeNode("Select")) {
        bool boxActive = IsBoxFilterActive();
        if (ImGui::Checkbox("BoxFilter", &boxActive)) {
            SetBoxFilterActive(boxActive);
        }
        
        bool grayActive = IsGrayscaleActive();
        if (ImGui::Checkbox("Grayscale", &grayActive)) {
            SetGrayscaleActive(grayActive);
        }
        
        bool sepiaActive = IsSepiaActive();
        if (ImGui::Checkbox("Sepia", &sepiaActive)) {
            SetSepiaActive(sepiaActive);
        }
        
        bool vignetteActive = IsVignetteActive();
        if (ImGui::Checkbox("Vignette", &vignetteActive)) {
            SetVignetteActive(vignetteActive);
        }
        
        ImGui::TreePop();
    }

    // 各アクティブなパスの固有パラメータのImGuiコントロールを順次呼び出す
    for (auto& pass : passes_) {
        if (pass->IsActive()) {
            pass->ImGuiControl();
        }
    }
    
    ImGui::End();
#endif
}

// ==========================================
// 各個別パスのアクティブ状態制御アクセサ
// ==========================================

void PostProcess::SetGrayscaleActive(bool active) {
    if (passes_.size() > kPassIndexGrayscale) {
        passes_[kPassIndexGrayscale]->SetActive(active);
    }
}

bool PostProcess::IsGrayscaleActive() const {
    return (passes_.size() > kPassIndexGrayscale) ? passes_[kPassIndexGrayscale]->IsActive() : false;
}

void PostProcess::SetSepiaActive(bool active) {
    if (passes_.size() > kPassIndexSepia) {
        passes_[kPassIndexSepia]->SetActive(active);
    }
}

bool PostProcess::IsSepiaActive() const {
    return (passes_.size() > kPassIndexSepia) ? passes_[kPassIndexSepia]->IsActive() : false;
}

void PostProcess::SetVignetteActive(bool active) {
    if (passes_.size() > kPassIndexVignette) {
        passes_[kPassIndexVignette]->SetActive(active);
    }
}

bool PostProcess::IsVignetteActive() const {
    return (passes_.size() > kPassIndexVignette) ? passes_[kPassIndexVignette]->IsActive() : false;
}

void PostProcess::SetBoxFilterActive(bool active) {
    if (passes_.size() > kPassIndexBoxFilter) {
        passes_[kPassIndexBoxFilter]->SetActive(active);
    }
}

bool PostProcess::IsBoxFilterActive() const {
    return (passes_.size() > kPassIndexBoxFilter) ? passes_[kPassIndexBoxFilter]->IsActive() : false;
}

void PostProcess::ClearEffects() {
    for (auto& pass : passes_) {
        if (pass) {
            pass->SetActive(false);
        }
    }
}

// ==========================================
// ビネットパラメータアクセサの転送
// ==========================================

void PostProcess::SetVignetteScale(float scale) {
    if (passes_.size() > kPassIndexVignette) {
        auto vignette = dynamic_cast<VignettePass*>(passes_[kPassIndexVignette].get());
        if (vignette) {
            vignette->SetScale(scale);
        }
    }
}

float PostProcess::GetVignetteScale() const {
    if (passes_.size() > kPassIndexVignette) {
        auto vignette = dynamic_cast<VignettePass*>(passes_[kPassIndexVignette].get());
        if (vignette) {
            return vignette->GetScale();
        }
    }
    return 0.0f;
}

void PostProcess::SetVignetteExponent(float exponent) {
    if (passes_.size() > kPassIndexVignette) {
        auto vignette = dynamic_cast<VignettePass*>(passes_[kPassIndexVignette].get());
        if (vignette) {
            vignette->SetExponent(exponent);
        }
    }
}

float PostProcess::GetVignetteExponent() const {
    if (passes_.size() > kPassIndexVignette) {
        auto vignette = dynamic_cast<VignettePass*>(passes_[kPassIndexVignette].get());
        if (vignette) {
            return vignette->GetExponent();
        }
    }
    return 0.0f;
}

void PostProcess::SetBoxFilterKernelRadius(int32_t radius) {
    if (passes_.size() > kPassIndexBoxFilter) {
        auto boxFilter = dynamic_cast<BoxFilterPass*>(passes_[kPassIndexBoxFilter].get());
        if (boxFilter) {
            boxFilter->SetKernelRadius(radius);
        }
    }
}

int32_t PostProcess::GetBoxFilterKernelRadius() const {
    if (passes_.size() > kPassIndexBoxFilter) {
        auto boxFilter = dynamic_cast<BoxFilterPass*>(passes_[kPassIndexBoxFilter].get());
        if (boxFilter) {
            return boxFilter->GetKernelRadius();
        }
    }
    return 1;
}
