#include "ModelManager.h"
#include "../../Function/Function.h"

ModelManager& ModelManager::GetInstance() {
    static ModelManager instance;
    return instance;
}

std::shared_ptr<ModelData> ModelManager::LoadModel(ID3D12Device* device, const std::string& directory, const std::string& filename) {
    std::string key = directory + "/" + filename;
    auto it = modelCache_.find(key);
    if (it != modelCache_.end()) return it->second;

    auto data = std::make_shared<ModelData>(LoadObjFile(directory, filename));
    modelCache_.emplace(key, data);
    return data;
}

void ModelManager::Clear() {
    modelCache_.clear();
}
