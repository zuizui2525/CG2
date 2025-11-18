#include "RootSignatureBuilder.h"
#include <cassert>

void RootSignatureBuilder::AddRootParameter(const D3D12_ROOT_PARAMETER& param) {
    params_.push_back(param);
}

void RootSignatureBuilder::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler) {
    samplers_.push_back(sampler);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(ID3D12Device* device) {

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = (UINT)params_.size();
    desc.pParameters = params_.data();

    desc.NumStaticSamplers = (UINT)samplers_.size();
    desc.pStaticSamplers = samplers_.data();

    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<ID3D12RootSignature> signature;
    hr = device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&signature)
    );

    assert(SUCCEEDED(hr));
    return signature;
}
