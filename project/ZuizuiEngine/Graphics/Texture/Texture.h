#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Function.h"

class Texture {
public:
    Texture();
    ~Texture();

    // 初期化（filePath が空なら白画像を使用）
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
        ID3D12DescriptorHeap* srvHeap, uint32_t descriptorIndex,
        const std::string& filePath);

    // 更新処理（今後の動的対応用）
    void Update();

    // 明示的リリース（任意）
    void Release();

    // GPUハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_;
    DirectX::TexMetadata metadata_;

    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

    std::string filePath_;
};
