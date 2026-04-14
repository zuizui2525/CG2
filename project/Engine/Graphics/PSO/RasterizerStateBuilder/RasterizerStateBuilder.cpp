#include "RasterizerStateBuilder.h"

RasterizerStateBuilder::RasterizerStateBuilder() {
    desc_ = {};
    desc_.FillMode = D3D12_FILL_MODE_SOLID;
    desc_.CullMode = D3D12_CULL_MODE_BACK;
    desc_.FrontCounterClockwise = FALSE;
    desc_.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    desc_.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    desc_.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    desc_.DepthClipEnable = TRUE;
    desc_.MultisampleEnable = FALSE;
    desc_.AntialiasedLineEnable = FALSE;
    desc_.ForcedSampleCount = 0;
    desc_.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void RasterizerStateBuilder::SetWireframe(bool enabled) {
    desc_.FillMode = enabled ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

void RasterizerStateBuilder::SetCullMode(CullMode mode) {
    switch (mode) {
    case CullMode::None:  desc_.CullMode = D3D12_CULL_MODE_NONE; break;
    case CullMode::Front: desc_.CullMode = D3D12_CULL_MODE_FRONT; break;
    case CullMode::Back:  desc_.CullMode = D3D12_CULL_MODE_BACK; break;
    }
}

void RasterizerStateBuilder::SetDepthBias(int bias) {
    desc_.DepthBias = bias;
    desc_.DepthBiasClamp = (bias != 0) ? 1.0f : 0.0f;
    desc_.SlopeScaledDepthBias = (bias != 0) ? 1.0f : 0.0f;
}

const D3D12_RASTERIZER_DESC& RasterizerStateBuilder::Build() {
    return desc_;
}
