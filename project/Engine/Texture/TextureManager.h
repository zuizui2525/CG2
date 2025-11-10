#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "Texture.h"

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager() = default;

    // 初期化
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap);

    // テクスチャをロードして登録（同じ名前ならスキップ）
    void LoadTexture(const std::string& name, const std::string& filePath);

    // 更新処理
    void Update();

    // GPUハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const std::string& name) const;

private:
    ID3D12Device* device_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr;
    ID3D12DescriptorHeap* srvHeap_ = nullptr;
    uint32_t descriptorCount_ = 1; // descriptor heapの先頭は別用途（0）なので1から開始

    std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;
};
