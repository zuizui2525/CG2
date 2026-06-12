#include "Engine/Graphics/PostProcess/UnderwaterPass.h"
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
    // Default parameters to prevent magic numbers
    const float kDefaultDistortionStrength = 0.003f;
    const float kDefaultDistortionFrequency = 12.0f;
    const float kDefaultBlurStrength = 1.5f;
    const float kDefaultBlurWeight = 0.7f;
    const float kDefaultWaterColorIntensity = 0.0f; // Default to 0.0 (disabled)
    const Vector3 kDefaultWaterColor = { 0.0f, 0.4f, 0.7f };

    constexpr UINT kRootParamIndexSRV = 0;
    constexpr UINT kRootParamIndexCBV = 1;
}

void UnderwaterPass::Initialize(ID3D12Device* device) {
    Zuizui* engine = Zuizui::GetInstance();
    DxCommon* dxCommon = engine->GetDxCommon();

    // 1. Create and map constant buffer
    paramsResource_ = DxUtils::CreateBufferResource(device, sizeof(UnderwaterParams));
    assert(paramsResource_ != nullptr && "Failed to create paramsResource_ in UnderwaterPass!");

    HRESULT hr = paramsResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramsData_));
    assert(SUCCEEDED(hr) && paramsData_ != nullptr && "Failed to map paramsResource_ in UnderwaterPass!");

    // Set default values
    paramsData_->time = 0.0f;
    paramsData_->distortionStrength = kDefaultDistortionStrength;
    paramsData_->distortionFrequency = kDefaultDistortionFrequency;
    paramsData_->blurStrength = kDefaultBlurStrength;
    paramsData_->blurWeight = kDefaultBlurWeight;
    paramsData_->waterColorIntensity = kDefaultWaterColorIntensity;
    paramsData_->pad = { 0.0f, 0.0f };
    paramsData_->waterColor = kDefaultWaterColor;
    paramsData_->pad2 = 0.0f;

    // 2. RootSignature Builder
    RootSignatureBuilder rs;
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_PIXEL); // t0 (PS)
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL); // b0 (PS)

    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    rs.AddSampler(sampler, 0);

    rootSignature_ = rs.Build(device);
    assert(rootSignature_ && "UnderwaterPass RootSignature creation failed!");

    // 3. Shader Compile
    bool vsResult = shaderProgram_.CompileVS(
        L"resources/Shader/Fullscreen/Fullscreen.VS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(vsResult && "UnderwaterPass VS Compile Failed!");

    bool psResult = shaderProgram_.CompilePS(
        L"resources/Shader/Fullscreen/Underwater.PS.hlsl",
        dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()
    );
    assert(psResult && "UnderwaterPass PS Compile Failed!");

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
    assert(SUCCEEDED(hr) && "Failed to create UnderwaterPass GraphicsPipelineState!");
}

void UnderwaterPass::Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) {
    if (!paramsData_) return;

    // Update time parameter dynamically
    float deltaTime = Zuizui::GetInstance()->GetDxCommon()->GetDeltaTime();
    accumTime_ += deltaTime;
    paramsData_->time = accumTime_;

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

void UnderwaterPass::ImGuiControl() {
#ifdef _USEIMGUI
    if (ImGui::TreeNode("Underwater")) {
        ImGui::DragFloat("Distortion Strength", &paramsData_->distortionStrength, 0.0001f, 0.0f, 0.05f, "%.4f");
        ImGui::DragFloat("Distortion Frequency", &paramsData_->distortionFrequency, 0.1f, 0.0f, 50.0f, "%.1f");
        ImGui::DragFloat("Blur Strength", &paramsData_->blurStrength, 0.05f, 0.0f, 10.0f, "%.2f");
        ImGui::DragFloat("Blur Weight", &paramsData_->blurWeight, 0.01f, 0.0f, 1.0f, "%.2f");
        
        ImGui::Separator();
        ImGui::DragFloat("Water Color Intensity", &paramsData_->waterColorIntensity, 0.01f, 0.0f, 1.0f, "%.2f");
        
        float colorArr[3] = { paramsData_->waterColor.x, paramsData_->waterColor.y, paramsData_->waterColor.z };
        if (ImGui::ColorEdit3("Water Color", colorArr)) {
            paramsData_->waterColor = { colorArr[0], colorArr[1], colorArr[2] };
        }

        ImGui::TreePop();
    }
#endif
}
