#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>

class RootSignatureBuilder {
public:
    RootSignatureBuilder() = default;
    ~RootSignatureBuilder() = default;

    // RootParameter を追加
    void AddRootParameter(const D3D12_ROOT_PARAMETER& param);

    // 静的サンプラーを追加
    void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler);

    // 実際に RootSignature を作成
    Microsoft::WRL::ComPtr<ID3D12RootSignature> Build(ID3D12Device* device);

private:
    std::vector<D3D12_ROOT_PARAMETER> params_;
    std::vector<D3D12_STATIC_SAMPLER_DESC> samplers_;
};
