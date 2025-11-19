#include "DepthStencilStateBuilder.h"

DepthStencilStateBuilder::DepthStencilStateBuilder() {
    // ゼロクリア
    desc_ = {};

    // --- 深度テストのデフォルト ---
    desc_.DepthEnable = TRUE;
    desc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    // --- ステンシルテスト（デフォルト無効） ---
    desc_.StencilEnable = FALSE;
    desc_.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc_.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    // フロント / バックフェイスのデフォルト
    D3D12_DEPTH_STENCILOP_DESC defaultOp = {};
    defaultOp.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    defaultOp.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    defaultOp.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    defaultOp.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    desc_.FrontFace = defaultOp;
    desc_.BackFace = defaultOp;
}

void DepthStencilStateBuilder::SetDepthEnable(bool enable) {
    desc_.DepthEnable = enable ? TRUE : FALSE;
}

void DepthStencilStateBuilder::SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask) {
    desc_.DepthWriteMask = mask;
}

void DepthStencilStateBuilder::SetDepthFunc(D3D12_COMPARISON_FUNC func) {
    desc_.DepthFunc = func;
}

void DepthStencilStateBuilder::SetStencilEnable(bool enable) {
    desc_.StencilEnable = enable ? TRUE : FALSE;
}

void DepthStencilStateBuilder::SetStencilReadMask(UINT8 mask) {
    desc_.StencilReadMask = mask;
}

void DepthStencilStateBuilder::SetStencilWriteMask(UINT8 mask) {
    desc_.StencilWriteMask = mask;
}
