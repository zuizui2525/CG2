#include "PSOPreset.h"
#include <cassert>
#include <iostream>

PSOPreset PSOPreset::CreateObject3DPreset(
    ID3D12Device* device,
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler) {

    PSOPreset preset;

    // ---------------------------
    // 1. RootSignature
    // ---------------------------
    RootSignatureBuilder rs;
    // 行列 (b0, VS)
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
    // マテリアル(b0, PS)
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);
    // ライティング (b1, PS)
    rs.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
    // テクスチャ (t0, PS)
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_PIXEL);
    // サンプラー (s0)
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    rs.AddSampler(sampler, 0);

    preset.rootSignature = rs.Build(device);
    assert(preset.rootSignature && "RootSignature creation failed!");

    // ---------------------------
    // 2. InputLayout
    // ---------------------------
    preset.ilBuilder_.Add("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT);
    preset.ilBuilder_.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);
    preset.ilBuilder_.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT);

    // ここでBuildするが、PSOManagerへのコピー後に無効化されるため、
    // Register時に再Buildが必要（PSOManager側で対応）
    preset.inputLayoutDesc = preset.ilBuilder_.Build();

    // ---------------------------
    // 3. Blend State
    // ---------------------------
    BlendStateBuilder blendBuilder;
    blendBuilder.SetBlendMode(kBlendModeNormal);
    preset.blendDesc = blendBuilder.Build();

    // ---------------------------
    // 4. Rasterizer State
    // ---------------------------
    RasterizerStateBuilder rsb;
    rsb.SetCullMode(CullMode::Back); // 通常はBackカリング推奨
    preset.rasterizerDesc = rsb.Build();

    // ---------------------------
    // 5. Depth Stencil State
    // ---------------------------
    DepthStencilStateBuilder dsb;
    dsb.SetDepthEnable(true);
    preset.depthStencilDesc = dsb.GetDesc();

    // ---------------------------
    // 6. Shader
    // ---------------------------
    // パスはプロジェクトの構成に合わせて正確に記述してください
    // VS
    bool vsResult = preset.shaderProgram.CompileVS(
        L"resources/Shader/Object3D.VS.hlsl",
        dxcUtils, dxcCompiler, includeHandler
    );
    // ★失敗したらここで止める
    assert(vsResult && "Vertex Shader Compile Failed! Check filepath or code.");

    // PS
    bool psResult = preset.shaderProgram.CompilePS(
        L"resources/Shader/Object3D.PS.hlsl",
        dxcUtils, dxcCompiler, includeHandler
    );
    // ★失敗したらここで止める
    assert(psResult && "Pixel Shader Compile Failed! Check filepath or code.");

    return preset;
}
