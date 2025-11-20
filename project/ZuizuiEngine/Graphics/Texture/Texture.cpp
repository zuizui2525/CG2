#include "Texture.h"

Texture::Texture() {}
Texture::~Texture() {
    Release();
}

void Texture::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
    ID3D12DescriptorHeap* srvHeap, uint32_t descriptorIndex,
    const std::string& filePath) {
    // 空パスなら白画像を使用
    filePath_ = filePath.empty() ? "resources/white.png" : filePath;

    // テクスチャ読み込み
    DirectX::ScratchImage mipImages = LoadTexture(filePath_);
    metadata_ = mipImages.GetMetadata();

    // テクスチャリソース作成
    textureResource_ = CreateTextureResource(device, metadata_);
    intermediateResource_ = UploadTextureData(textureResource_.Get(), mipImages, device, commandList);

    // SRV設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = metadata_.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(metadata_.mipLevels);

    // ディスクリプタ計算
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    textureSrvHandleCPU_ = GetCPUDescriptorHandle(srvHeap, descriptorSize, descriptorIndex);
    textureSrvHandleGPU_ = GetGPUDescriptorHandle(srvHeap, descriptorSize, descriptorIndex);

    // SRV作成
    device->CreateShaderResourceView(textureResource_.Get(), &srvDesc, textureSrvHandleCPU_);
}

void Texture::Update() {
    // 今後の動的テクスチャ更新処理に利用可能
}

void Texture::Release() {
    textureResource_.Reset();
    intermediateResource_.Reset();
    textureSrvHandleCPU_ = {};
    textureSrvHandleGPU_ = {};
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetGpuHandle() const {
    if (textureSrvHandleGPU_.ptr == 0) {
        D3D12_GPU_DESCRIPTOR_HANDLE nullHandle{};
        nullHandle.ptr = 0;
        return nullHandle;
    }
    return textureSrvHandleGPU_;
}
