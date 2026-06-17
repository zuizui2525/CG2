#include "Engine/Graphics/PostProcess/RadialBlurPass.h"
#include "Engine/Graphics/PSO/RootSignature/RootSignatureBuilder.h"
#include "Engine/Graphics/PSO/BlendState/BlendStateBuilder.h"
#include "Engine/Graphics/PSO/RasterizerState/RasterizerStateBuilder.h"
#include "Engine/Graphics/PSO/DepthStencilState/DepthStencilStateBuilder.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include <cassert>

#ifdef _USEIMGUI
#include <imgui.h>
#endif

namespace {
    // デフォルトパラメータの定数定義
    const Vector2 kDefaultCenter = { 0.5f, 0.5f };
    constexpr float kDefaultBlurWidth = 0.01f;

    // ルートパラメータインデックス
    constexpr UINT kRootParamIndexSRV = 0; // t0
    constexpr UINT kRootParamIndexCBV = 1; // b0
}

void RadialBlurPass::Initialize(ID3D12Device* device) {
    Zuizui* engine = Zuizui::GetInstance();
    DxCommon* dxCommon = engine->GetDxCommon();

    // 1. 定数バッファの作成と初期化
    paramsResource_ = DxUtils::CreateBufferResource(device, sizeof(RadialBlurParams));
    assert(paramsResource_ != nullptr && "Failed to create paramsResource_ in RadialBlurPass!");

    HRESULT hr = paramsResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
    assert(SUCCEEDED(hr) && paramsData_ != nullptr && "Failed to map paramsResource_ in RadialBlurPass!");

    paramsData_->center = kDefaultCenter;
    paramsData_->blurWidth = kDefaultBlurWidth;
    paramsData_->pad = 0.0f;

    // 2. RootSignature
    RootSignatureBuilder rs;
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_PIXEL); // t0
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL); // b0

    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    rs.AddSampler(sampler, 0);

    rootSignature_ = rs.Build(device);
    assert(rootSignature_ && "RadialBlurPass RootSignature creation failed!");

    // 3. Shader Compile
    bool vsResult = shaderProgram_.CompileVS(
        L"resources/Shader/Fullscreen/Fullscreen.VS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(vsResult && "RadialBlurPass VS Compile Failed!");

    bool psResult = shaderProgram_.CompilePS(
        L"resources/Shader/Fullscreen/RadialBlur.PS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(psResult && "RadialBlurPass PS Compile Failed!");

    // 4. Pipeline State (PSO)
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
    assert(SUCCEEDED(hr) && "Failed to create RadialBlurPass GraphicsPipelineState!");
}

void RadialBlurPass::Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) {
    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetGraphicsRootDescriptorTable(kRootParamIndexSRV, inputSRV);
    commandList->SetGraphicsRootConstantBufferView(kRootParamIndexCBV, paramsResource_->GetGPUVirtualAddress());

    constexpr UINT kVertexCount = 3;
    constexpr UINT kInstanceCount = 1;
    constexpr UINT kStartVertexLocation = 0;
    constexpr UINT kStartInstanceLocation = 0;
    commandList->DrawInstanced(kVertexCount, kInstanceCount, kStartVertexLocation, kStartInstanceLocation);
}

void RadialBlurPass::ImGuiControl() {
#ifdef _USEIMGUI
    if (ImGui::TreeNode("RadialBlur")) {
        float centerArray[2] = { paramsData_->center.x, paramsData_->center.y };
        if (ImGui::SliderFloat2("Center", centerArray, 0.0f, 1.0f, "%.2f")) {
            paramsData_->center.x = centerArray[0];
            paramsData_->center.y = centerArray[1];
        }
        ImGui::DragFloat("Blur Width", &paramsData_->blurWidth, 0.001f, 0.0f, 0.1f, "%.3f");
        ImGui::TreePop();
    }
#endif
}
