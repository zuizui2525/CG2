#include "Engine/Graphics/PostProcess/TVNoisePass.h"
#include "Engine/Graphics/PSO/RootSignature/RootSignatureBuilder.h"
#include "Engine/Graphics/PSO/BlendState/BlendStateBuilder.h"
#include "Engine/Graphics/PSO/RasterizerState/RasterizerStateBuilder.h"
#include "Engine/Graphics/PSO/DepthStencilState/DepthStencilStateBuilder.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Zuizui.h"
#include <cassert>

#ifdef _USEIMGUI
#include <imgui.h>
#endif

namespace {
    // デフォルトパラメータ (マジックナンバー排除)
    const float kDefaultNoiseStrength = 0.5f;
    const float kDefaultTime = 0.0f;

    // ルートパラメータインデックス
    constexpr UINT kRootParamIndexSRV = 0;
    constexpr UINT kRootParamIndexCBV = 1;

    // 描画用の頂点数などの定数
    constexpr UINT kVertexCount = 3;
    constexpr UINT kInstanceCount = 1;
    constexpr UINT kStartVertexLocation = 0;
    constexpr UINT kStartInstanceLocation = 0;

    // スライダー等の範囲定数
    const float kMinStrength = 0.0f;
    const float kMaxStrength = 1.0f;
}

void TVNoisePass::Initialize(ID3D12Device* device) {
    Zuizui* engine = Zuizui::GetInstance();
    DxCommon* dxCommon = engine->GetDxCommon();

    // 1. 定数バッファの作成とマップ
    paramsResource_ = DxUtils::CreateBufferResource(device, sizeof(TVNoiseParams));
    assert(paramsResource_ != nullptr && "Failed to create paramsResource_ in TVNoisePass!");

    HRESULT hr = paramsResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
    assert(SUCCEEDED(hr) && paramsData_ != nullptr && "Failed to map paramsResource_ in TVNoisePass!");

    // デフォルト設定
    paramsData_->time = kDefaultTime;
    paramsData_->noiseStrength = kDefaultNoiseStrength;
    paramsData_->pad = { 0.0f, 0.0f };

    // 2. RootSignature Builder
    RootSignatureBuilder rs;
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_PIXEL); // t0: 元画像テクスチャ
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL); // b0: ノイズパラメータ

    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    rs.AddSampler(sampler, 0);

    rootSignature_ = rs.Build(device);
    assert(rootSignature_ && "TVNoisePass RootSignature creation failed!");

    // 3. シェーダーコンパイル
    bool vsResult = shaderProgram_.CompileVS(
        L"resources/Shader/Fullscreen/Fullscreen.VS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(vsResult && "TVNoisePass VS Compile Failed!");

    bool psResult = shaderProgram_.CompilePS(
        L"resources/Shader/Fullscreen/TVNoise.PS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(psResult && "TVNoisePass PS Compile Failed!");

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
    assert(SUCCEEDED(hr) && "Failed to create TVNoisePass GraphicsPipelineState!");
}

void TVNoisePass::Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) {
    if (!paramsData_) return;

    // 時間更新処理
    float deltaTime = Zuizui::GetInstance()->GetDxCommon()->GetDeltaTime();
    accumTime_ += deltaTime;
    paramsData_->time = accumTime_;

    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetGraphicsRootDescriptorTable(kRootParamIndexSRV, inputSRV);
    commandList->SetGraphicsRootConstantBufferView(kRootParamIndexCBV, paramsResource_->GetGPUVirtualAddress());

    commandList->DrawInstanced(kVertexCount, kInstanceCount, kStartVertexLocation, kStartInstanceLocation);
}

void TVNoisePass::ImGuiControl() {
#ifdef _USEIMGUI
    if (ImGui::TreeNode("TV Noise")) {
        ImGui::SliderFloat("Strength", &paramsData_->noiseStrength, kMinStrength, kMaxStrength);
        ImGui::TreePop();
    }
#endif
}

void TVNoisePass::SetNoiseStrength(float strength) {
    if (paramsData_) {
        paramsData_->noiseStrength = strength;
    }
}

float TVNoisePass::GetNoiseStrength() const {
    return paramsData_ ? paramsData_->noiseStrength : kDefaultNoiseStrength;
}
