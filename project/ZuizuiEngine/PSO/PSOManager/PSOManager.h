#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <dxcapi.h>

#include "../PSOPreset/PSOPreset.h"

class PSOManager {
public:
    PSOManager(ID3D12Device* device);
    ~PSOManager() = default;

    HRESULT Initialize(
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler);

    // 指定したプリセットのブレンドモードを変更・更新
    HRESULT UpdateBlendMode(const std::string& name, BlendMode mode);

    ID3D12PipelineState* GetPSO(const std::string& name) const;
    ID3D12RootSignature* GetRootSignature(const std::string& name) const;

private:
    // 内部用: プリセットを登録してPSO生成
    void RegisterPreset(const std::string& name, const PSOPreset& preset);

    struct PSOData {
        PSOPreset originalPreset;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    };

    ID3D12Device* device_;
    std::unordered_map<std::string, PSOData> psoMap_;
};
