#include "BlendStateBuilder.h"

BlendStateBuilder::BlendStateBuilder() {
    // デフォルト（ブレンドなし）
    blendDesc_.AlphaToCoverageEnable = FALSE;
    blendDesc_.IndependentBlendEnable = FALSE;

    auto& rt0 = blendDesc_.RenderTarget[0];
    rt0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    rt0.BlendEnable = FALSE;
}

void BlendStateBuilder::SetBlendMode(BlendMode mode) {
    auto& rt0 = blendDesc_.RenderTarget[0];

    switch (mode) {
    case kBlendModeNone:
        rt0.BlendEnable = FALSE;
        break;

    case kBlendModeNormal:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        break;

    case kBlendModeAdd:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0.DestBlend = D3D12_BLEND_ONE;
        break;

    case kBlendModeSubtract:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0.DestBlend = D3D12_BLEND_ONE;
        break;

    case kBlendModeMultiply:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_ZERO;
        rt0.DestBlend = D3D12_BLEND_SRC_COLOR;
        break;

    case kBlendModeScreen:
        rt0.BlendEnable = TRUE;
        rt0.BlendOp = D3D12_BLEND_OP_ADD;
        rt0.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        rt0.DestBlend = D3D12_BLEND_ONE;
        break;
    }
}

const D3D12_BLEND_DESC& BlendStateBuilder::Build() {
    return blendDesc_;
}
