#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <fstream>
#include "Struct.h"
#include "Matrix.h"

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

class PSO {
public:
    // ComPtrで管理
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Vector4* materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    Matrix4x4* wvpData_ = nullptr;

public:
    PSO(
        ID3D12Device* device,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler,
        std::ofstream& logStream
    );

    // デストラクタはComPtrが自動で解放するため空で良い
    ~PSO() = default;

    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }
};