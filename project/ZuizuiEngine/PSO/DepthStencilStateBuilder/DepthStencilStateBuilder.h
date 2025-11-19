#pragma once
#include <d3d12.h>

class DepthStencilStateBuilder {
public:
    DepthStencilStateBuilder();

    // 設定反映
    void SetDepthEnable(bool enable);
    void SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask);
    void SetDepthFunc(D3D12_COMPARISON_FUNC func);

    void SetStencilEnable(bool enable);
    void SetStencilReadMask(UINT8 mask);
    void SetStencilWriteMask(UINT8 mask);

    // 最終取得
    const D3D12_DEPTH_STENCIL_DESC& GetDesc() const { return desc_; }

private:
    D3D12_DEPTH_STENCIL_DESC desc_;
};
