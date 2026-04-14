#include "TextureManager.h"
#include "Zuizui.h"
#include "BaseResource.h"

void TextureManager::Initialize() {
    // Engine
    auto engine = EngineResource::GetEngine();
    assert(engine != nullptr);

    device_ = engine->GetDevice();
    commandList_ = engine->GetDxCommon()->GetCommandList();
    srvHeap_ = engine->GetDxCommon()->GetSrvHeap();
}

void TextureManager::LoadTexture(const std::string& name, const std::string& filePath) {
    // すでにロード済みならスキップ
    if (textures_.find(name) != textures_.end()) return;

    auto texture = std::make_unique<Texture>();
    texture->Initialize(device_, commandList_, srvHeap_, descriptorCount_, filePath);
    textures_[name] = std::move(texture);
    descriptorCount_++;
}

void TextureManager::Update() {
    for (auto& tex : textures_) {
        tex.second->Update();
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGpuHandle(const std::string& name) const {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return it->second->GetGpuHandle();
    }

    // 存在しない場合のフォールバック（nullptr的ハンドル）
    D3D12_GPU_DESCRIPTOR_HANDLE nullHandle{};
    nullHandle.ptr = 0;
    return nullHandle;
}
