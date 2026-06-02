#include "Engine/Graphics/PSO/BlendState/BlendStateBuilder.h"

BlendStateBuilder::BlendStateBuilder() {
    // デフォルト（ブレンドなし）
    blendDesc_.AlphaToCoverageEnable = FALSE;
    blendDesc_.IndependentBlendEnable = FALSE;

    auto& rt0 = blendDesc_.RenderTarget[0];
    rt0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    rt0.BlendEnable = FALSE;

    // ★追加: コンストラクタでも無効な値(0)のままにしないよう、安全な初期値を入れておく
    rt0.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt0.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
}

void BlendStateBuilder::SetBlendMode(BlendMode mode) {
    auto& rt0 = blendDesc_.RenderTarget[0];

    // 毎回リセット記述するのが面倒なら、ここで共通設定としてAlpha設定を入れてもOK
    // 今回はswitch内で丁寧に書きます

    switch (mode) {
    case kBlendModeNone:
        rt0.BlendEnable = FALSE;
        // 無効時でも念のため有効な値を入れておくのが安全
        rt0.SrcBlend = D3D12_BLEND_ONE;
        rt0.DestBlend = D3D12_BLEND_ZERO;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt0.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case kBlendModeNormal:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

        // ★修正: レンダーターゲットのアルファ値を維持（上書きしない）
        rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt0.DestBlendAlpha = D3D12_BLEND_ONE;
        rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case kBlendModeAdd:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0.DestBlend = D3D12_BLEND_ONE;

        // ★修正: レンダーターゲットのアルファ値を維持（上書きしない）
        rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt0.DestBlendAlpha = D3D12_BLEND_ONE;
        rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case kBlendModeSubtract:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0.DestBlend = D3D12_BLEND_ONE;

        // ★修正: レンダーターゲットのアルファ値を維持（上書きしない）
        rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt0.DestBlendAlpha = D3D12_BLEND_ONE;
        rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case kBlendModeMultiply:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_ZERO;
        rt0.DestBlend = D3D12_BLEND_SRC_COLOR;

        // ★修正: レンダーターゲットのアルファ値を維持（上書きしない）
        rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt0.DestBlendAlpha = D3D12_BLEND_ONE;
        rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case kBlendModeScreen:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        rt0.DestBlend = D3D12_BLEND_ONE;

        // ★修正: レンダーターゲットのアルファ値を維持（上書きしない）
        rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt0.DestBlendAlpha = D3D12_BLEND_ONE;
        rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;
    }
}

const D3D12_BLEND_DESC& BlendStateBuilder::Build() {
    return blendDesc_;
}
