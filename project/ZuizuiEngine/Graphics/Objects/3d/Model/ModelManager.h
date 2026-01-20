#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <d3d12.h>
#include "Struct.h"

class TextureManager;

class ModelManager {
public:
    ModelManager() = default;
    ~ModelManager() = default;

    void Initialize(ID3D12Device* device, TextureManager* texMgr);
    void LoadModel(const std::string& name, const std::string& filename);
    std::shared_ptr<ModelData> GetModelData(const std::string& name) const;
    void Clear();

private:
    ID3D12Device* device_ = nullptr;
    TextureManager* texMgr_ = nullptr;
    std::unordered_map<std::string, std::shared_ptr<ModelData>> models_;
};
