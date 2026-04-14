#pragma once
#include <d3d12.h>

enum class CullMode {
    None,
    Front,
    Back
};

class RasterizerStateBuilder {
public:
    RasterizerStateBuilder();
    ~RasterizerStateBuilder() = default;

    void SetWireframe(bool enabled);
    void SetCullMode(CullMode mode);
    void SetDepthBias(int bias);

    const D3D12_RASTERIZER_DESC& Build();

private:
    D3D12_RASTERIZER_DESC desc_;
};
