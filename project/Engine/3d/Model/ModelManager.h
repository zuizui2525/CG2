#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <d3d12.h>
#include "../../Struct.h"

class ModelManager {
public:
    static ModelManager& GetInstance();
    std::shared_ptr<ModelData> LoadModel(ID3D12Device* device, const std::string& directory, const std::string& filename);
    void Clear();

private:
    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

private:
    std::unordered_map<std::string, std::shared_ptr<ModelData>> modelCache_;
};
