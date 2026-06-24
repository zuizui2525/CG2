#include "Engine/Graphics/PostProcess/DissolvePass.h"
#include "Engine/Graphics/PSO/RootSignature/RootSignatureBuilder.h"
#include "Engine/Graphics/PSO/BlendState/BlendStateBuilder.h"
#include "Engine/Graphics/PSO/RasterizerState/RasterizerStateBuilder.h"
#include "Engine/Graphics/PSO/DepthStencilState/DepthStencilStateBuilder.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Zuizui.h"
#include <cassert>

#ifdef _USEIMGUI
#include <imgui.h>
#endif

namespace {
    // デフォルトパラメータ (マジックナンバー排除)
    const float kDefaultThreshold = 0.0f;
    const float kDefaultEdgeWidth = 0.05f;
    const Vector3 kDefaultEdgeColor = { 1.0f, 0.5f, 0.0f }; // オレンジ発光

    // ルートパラメータインデックス
    constexpr UINT kRootParamIndexSRV = 0;
    constexpr UINT kRootParamIndexNoiseSRV = 1;
    constexpr UINT kRootParamIndexCBV = 2;

    // 描画用の頂点数などの定数
    constexpr UINT kVertexCount = 3;
    constexpr UINT kInstanceCount = 1;
    constexpr UINT kStartVertexLocation = 0;
    constexpr UINT kStartInstanceLocation = 0;

    // スライダー等の範囲定数
    const float kMinThreshold = 0.0f;
    const float kMaxThreshold = 1.0f;
    const float kMinEdgeWidth = 0.0f;
    const float kMaxEdgeWidth = 0.2f;

    // ノイズテクスチャキーの定義とImGui表示用名
    const std::vector<std::string> kNoiseTextureKeys = { "noise0", "noise1", "noise2" };
    const char* kNoiseNames[] = { "Noise 0", "Noise 1", "Noise 2" };
}

void DissolvePass::Initialize(ID3D12Device* device) {
    Zuizui* engine = Zuizui::GetInstance();
    DxCommon* dxCommon = engine->GetDxCommon();

    // 1. 定数バッファの作成とマップ
    paramsResource_ = DxUtils::CreateBufferResource(device, sizeof(DissolveParams));
    assert(paramsResource_ != nullptr && "Failed to create paramsResource_ in DissolvePass!");

    HRESULT hr = paramsResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
    assert(SUCCEEDED(hr) && paramsData_ != nullptr && "Failed to map paramsResource_ in DissolvePass!");

    // デフォルト設定
    paramsData_->threshold = kDefaultThreshold;
    paramsData_->edgeWidth = kDefaultEdgeWidth;
    paramsData_->pad = { 0.0f, 0.0f };
    paramsData_->edgeColor = kDefaultEdgeColor;
    paramsData_->pad2 = 0.0f;

    // 2. RootSignature Builder
    RootSignatureBuilder rs;
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_PIXEL); // t0: 元画像テクスチャ
    rs.AddSRV(1, D3D12_SHADER_VISIBILITY_PIXEL); // t1: ノイズテクスチャ
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL); // b0: ディゾルブパラメータ

    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 繰り返し可能に
    rs.AddSampler(sampler, 0);

    rootSignature_ = rs.Build(device);
    assert(rootSignature_ && "DissolvePass RootSignature creation failed!");

    // 3. シェーダーコンパイル
    bool vsResult = shaderProgram_.CompileVS(
        L"resources/Shader/Fullscreen/Fullscreen.VS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(vsResult && "DissolvePass VS Compile Failed!");

    bool psResult = shaderProgram_.CompilePS(
        L"resources/Shader/Fullscreen/Dissolve.PS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(psResult && "DissolvePass PS Compile Failed!");

    // 4. PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_.Get();
    desc.InputLayout.pInputElementDescs = nullptr;
    desc.InputLayout.NumElements = 0;
    desc.VS = shaderProgram_.GetVS();
    desc.PS = shaderProgram_.GetPS();

    BlendStateBuilder blendBuilder;
    blendBuilder.SetBlendMode(kBlendModeNone);
    desc.BlendState = blendBuilder.Build();

    RasterizerStateBuilder rsb;
    rsb.SetCullMode(CullMode::None);
    desc.RasterizerState = rsb.Build();

    DepthStencilStateBuilder dsb;
    dsb.SetDepthEnable(false);
    desc.DepthStencilState = dsb.GetDesc();

    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;

    hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
    assert(SUCCEEDED(hr) && "Failed to create DissolvePass GraphicsPipelineState!");
}

void DissolvePass::Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) {
    if (!paramsData_) return;

    TextureManager* texMgr = TextureResource::GetTextureManager();
    assert(texMgr != nullptr);

    // インデックスの範囲チェックを行い、安全にキー名を取得
    std::string noiseKey = "noise0";
    if (activeNoiseIndex_ >= 0 && activeNoiseIndex_ < static_cast<int32_t>(kNoiseTextureKeys.size())) {
        noiseKey = kNoiseTextureKeys[activeNoiseIndex_];
    }
    D3D12_GPU_DESCRIPTOR_HANDLE noiseSRV = texMgr->GetGpuHandle(noiseKey);

    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetGraphicsRootDescriptorTable(kRootParamIndexSRV, inputSRV);
    commandList->SetGraphicsRootDescriptorTable(kRootParamIndexNoiseSRV, noiseSRV);
    commandList->SetGraphicsRootConstantBufferView(kRootParamIndexCBV, paramsResource_->GetGPUVirtualAddress());

    commandList->DrawInstanced(kVertexCount, kInstanceCount, kStartVertexLocation, kStartInstanceLocation);
}

void DissolvePass::ImGuiControl() {
#ifdef _USEIMGUI
    if (ImGui::TreeNode("Dissolve")) {
        ImGui::SliderFloat("Threshold", &paramsData_->threshold, kMinThreshold, kMaxThreshold);
        ImGui::SliderFloat("Edge Width", &paramsData_->edgeWidth, kMinEdgeWidth, kMaxEdgeWidth);
        
        float colorArr[3] = { paramsData_->edgeColor.x, paramsData_->edgeColor.y, paramsData_->edgeColor.z };
        if (ImGui::ColorEdit3("Edge Color", colorArr)) {
            paramsData_->edgeColor = { colorArr[0], colorArr[1], colorArr[2] };
        }

        // ノイズの選択コンボボックス
        int currentItem = activeNoiseIndex_;
        int numItems = static_cast<int>(std::size(kNoiseNames));
        if (ImGui::Combo("Noise Type", &currentItem, kNoiseNames, numItems)) {
            SetActiveNoiseIndex(currentItem);
        }

        ImGui::TreePop();
    }
#endif
}

void DissolvePass::SetThreshold(float threshold) {
    if (paramsData_) {
        paramsData_->threshold = threshold;
    }
}

float DissolvePass::GetThreshold() const {
    return paramsData_ ? paramsData_->threshold : kDefaultThreshold;
}

void DissolvePass::SetEdgeWidth(float width) {
    if (paramsData_) {
        paramsData_->edgeWidth = width;
    }
}

float DissolvePass::GetEdgeWidth() const {
    return paramsData_ ? paramsData_->edgeWidth : kDefaultEdgeWidth;
}

void DissolvePass::SetEdgeColor(const Vector3& color) {
    if (paramsData_) {
        paramsData_->edgeColor = color;
    }
}

Vector3 DissolvePass::GetEdgeColor() const {
    return paramsData_ ? paramsData_->edgeColor : kDefaultEdgeColor;
}

void DissolvePass::SetActiveNoiseIndex(int32_t index) {
    int numItems = static_cast<int>(kNoiseTextureKeys.size());
    if (index >= 0 && index < numItems) {
        activeNoiseIndex_ = index;
    }
}

int32_t DissolvePass::GetActiveNoiseIndex() const {
    return activeNoiseIndex_;
}
