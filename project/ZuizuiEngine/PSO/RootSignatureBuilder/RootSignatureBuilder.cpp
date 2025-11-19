#include "RootSignatureBuilder.h"

RootSignatureBuilder::RootSignatureBuilder() {
    rootParams_.clear();
    samplers_.clear();
    descriptors_.clear();
}

void RootSignatureBuilder::AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    descriptors_.push_back({});
    descriptors_.back().ShaderRegister = shaderRegister;
    descriptors_.back().RegisterSpace = 0;

    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = visibility;
    param.Descriptor = descriptors_.back();

    rootParams_.push_back(param);
}

void RootSignatureBuilder::AddSRV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    descriptors_.push_back({});
    descriptors_.back().ShaderRegister = shaderRegister;
    descriptors_.back().RegisterSpace = 0;

    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.ShaderVisibility = visibility;
    param.Descriptor = descriptors_.back();

    rootParams_.push_back(param);
}

void RootSignatureBuilder::AddSampler(const D3D12_SAMPLER_DESC& samplerDesc, UINT shaderRegister) {
    D3D12_STATIC_SAMPLER_DESC s{};
    s.Filter = samplerDesc.Filter;
    s.AddressU = samplerDesc.AddressU;
    s.AddressV = samplerDesc.AddressV;
    s.AddressW = samplerDesc.AddressW;
    s.MipLODBias = samplerDesc.MipLODBias;
    s.MaxAnisotropy = samplerDesc.MaxAnisotropy;
    s.ComparisonFunc = samplerDesc.ComparisonFunc;
    s.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    s.MinLOD = samplerDesc.MinLOD;
    s.MaxLOD = samplerDesc.MaxLOD;
    s.ShaderRegister = shaderRegister;
    s.RegisterSpace = 0;
    s.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    samplers_.push_back(s);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(ID3D12Device* device) {
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = static_cast<UINT>(rootParams_.size());
    desc.pParameters = rootParams_.data();
    desc.NumStaticSamplers = static_cast<UINT>(samplers_.size());
    desc.pStaticSamplers = samplers_.data();
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        return nullptr;
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
    device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSig));

    return rootSig;
}
