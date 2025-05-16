#pragma once
#include "Header.h"

class PSO {
public:
    // 生ポインタで管理
    ID3D12RootSignature* rootSignature_ = nullptr;
    ID3D12PipelineState* graphicsPipelineState_ = nullptr;

    ID3D12Resource* materialResource_ = nullptr;
    Vector4* materialData_ = nullptr;

    ID3D12Resource* wvpResource_ = nullptr;
    Matrix4x4* wvpData_ = nullptr;

public:
    PSO(
        ID3D12Device* device,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler,
        std::ofstream& logStream
    );

    ~PSO();

    ID3D12RootSignature* GetRootSignature() const { return rootSignature_; }
    ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_; }
};
