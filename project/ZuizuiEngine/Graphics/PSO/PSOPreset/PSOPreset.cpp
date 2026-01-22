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
    // カメラ (b1, PS)
    rs.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);
    // ライティング (b2, PS) directionalLight
    rs.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    // ライティング (b3, PS) pointLight
    rs.AddCBV(3, D3D12_SHADER_VISIBILITY_PIXEL);
    // ライティング (b4, PS) spotLight
    rs.AddCBV(4, D3D12_SHADER_VISIBILITY_PIXEL);
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
        L"resources/Shader/Object3d/Object3d.VS.hlsl",
        dxcUtils, dxcCompiler, includeHandler
    );
    // ★失敗したらここで止める
    assert(vsResult && "Vertex Shader Compile Failed! Check filepath or code.");

    // PS
    bool psResult = preset.shaderProgram.CompilePS(
        L"resources/Shader/Object3d/Object3d.PS.hlsl",
        dxcUtils, dxcCompiler, includeHandler
    );
    // ★失敗したらここで止める
    assert(psResult && "Pixel Shader Compile Failed! Check filepath or code.");

    return preset;
}

PSOPreset PSOPreset::CreateParticlePreset(
    ID3D12Device* device,
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler) {

    PSOPreset preset;

    // --------------------------------------------------------
    // 1. RootSignature の構築 (Object3Dとの最大の違い)
    // --------------------------------------------------------
    RootSignatureBuilder rs;

    // ■ 行列データ (VS)
    // Object3Dでは CBV(b0) でしたが、
    // Instancingでは StructuredBuffer(t0) を使うため SRV に変更します。
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_VERTEX); // register(t0)

    // ■ マテリアル (PS)
    // これはObject3Dと同じく定数バッファ(b0)
    rs.AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL);  // register(b0)

    // ■ DirectionalLight (PS)
    // シェーダーに残っていたライティング用(b1)
    rs.AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL);  // register(b1)

    // ■ Texture (PS)
    // テクスチャ(t0)。VSのt0とはシェーダーステージが違うので被ってもOK
    rs.AddSRV(0, D3D12_SHADER_VISIBILITY_PIXEL);  // register(t0)

    // ■ Sampler
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    rs.AddSampler(sampler, 0);

    // RootSignatureをビルド
    preset.rootSignature = rs.Build(device);


    // --------------------------------------------------------
    // 2. Input Layout
    // --------------------------------------------------------
    preset.ilBuilder_.Add("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT);
    preset.ilBuilder_.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);
    preset.ilBuilder_.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT);
    preset.inputLayoutDesc = preset.ilBuilder_.Build();


    // --------------------------------------------------------
    // 3. Blend State
    // --------------------------------------------------------
    BlendStateBuilder blendBuilder;
    // パーティクルなので「加算合成(kBlendModeAdd)」がよく使われますが、
    // まずは動作確認のため「通常(Normal)」にしておきます。
    blendBuilder.SetBlendMode(kBlendModeAdd);
    preset.blendDesc = blendBuilder.Build();


    // --------------------------------------------------------
    // 4. Rasterizer State
    // --------------------------------------------------------
    RasterizerStateBuilder rsb;
    rsb.SetCullMode(CullMode::Back); // 裏面を表示しない
    preset.rasterizerDesc = rsb.Build();


    // --------------------------------------------------------
    // 5. Depth Stencil State
    // --------------------------------------------------------
    DepthStencilStateBuilder dsb;
    dsb.SetDepthEnable(true);
    // 半透明パーティクルの場合、深度書き込み(DepthWrite)をOFF
    dsb.SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO); 
    preset.depthStencilDesc = dsb.GetDesc();


    // --------------------------------------------------------
    // 6. Shader Compile (★ファイル名をParticle用に)
    // --------------------------------------------------------
    bool vsResult = preset.shaderProgram.CompileVS(
        L"resources/Shader/Particle/Particle.VS.hlsl", // ★Particle用のVS
        dxcUtils, dxcCompiler, includeHandler
    );
    // コンパイルエラーチェック
    assert(vsResult && "Particle VS Compile Failed!");

    bool psResult = preset.shaderProgram.CompilePS(
        L"resources/Shader/Particle/Particle.PS.hlsl", // ★Particle用のPS
        dxcUtils, dxcCompiler, includeHandler
    );
    assert(psResult && "Particle PS Compile Failed!");

    return preset;
}
