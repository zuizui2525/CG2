#pragma once
#include "Header.h"

class PSO {
public:
    PSO(
        ID3D12Device* device,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler,
        std::ofstream& logStream
    );

    ~PSO();

    ID3D12RootSignature* GetRootSignature() const { return rootSignature; }
    ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState; }

    // 解放はmain.cppで行うため、デストラクタは空にするか、解放しない

public:
    // 生ポインタで管理
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* graphicsPipelineState = nullptr;

    ID3D12Resource* materialResource = nullptr;
    Vector4* materialData = nullptr;

    ID3D12Resource* wvpResource = nullptr;
    Matrix4x4* wvpData = nullptr;
};
