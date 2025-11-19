#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>

class RootSignatureBuilder {
public:
    RootSignatureBuilder();

    // ルートパラメータ追加系
    void AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void AddSRV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    void AddSampler(const D3D12_SAMPLER_DESC& samplerDesc, UINT shaderRegister = 0);

    // ルートシグネチャ生成
    Microsoft::WRL::ComPtr<ID3D12RootSignature> Build(ID3D12Device* device);

private:
    std::vector<D3D12_ROOT_PARAMETER> rootParams_;
    std::vector<D3D12_STATIC_SAMPLER_DESC> samplers_;

    // メモリ保持用（名前・構造体の寿命確保）
    std::vector<D3D12_ROOT_DESCRIPTOR> descriptors_;
};
