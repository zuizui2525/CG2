#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>

// Builder群のインクルード
#include "RootSignatureBuilder.h"
#include "InputLayoutBuilder.h"
#include "BlendStateBuilder.h"
#include "RasterizerStateBuilder.h"
#include "DepthStencilStateBuilder.h"
#include "ShaderProgram.h"

class PSOPreset {
public:
    // Object3D用のプリセット作成関数
    static PSOPreset CreateObject3DPreset(
        ID3D12Device* device,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler);

    // Particle用のプリセット作成関数
    static PSOPreset CreateParticlePreset(
        ID3D12Device* device,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler);

    // メンバ変数
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    // パイプラインステートの設定構造体
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    D3D12_BLEND_DESC blendDesc{};
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

    // シェーダークラス
    ShaderProgram shaderProgram;

    // InputLayout再構築のためにBuilderを持っておく必要がある
    InputLayoutBuilder ilBuilder_;
};
