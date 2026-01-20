#include "ModelManager.h"
#include "Function.h"

void ModelManager::Initialize(ID3D12Device* device) {
    device_ = device;
}

void ModelManager::LoadModel(const std::string& name, const std::string& filename) {
    if (models_.find(name) != models_.end()) return;

    auto data = std::make_shared<ModelData>(LoadObjFile(filename));

    // 頂点リソース作成
    data->vertexResource = CreateBufferResource(device_, sizeof(VertexData) * data->vertices.size());
    data->vbv.BufferLocation = data->vertexResource->GetGPUVirtualAddress();
    data->vbv.SizeInBytes = sizeof(VertexData) * (UINT)data->vertices.size();
    data->vbv.StrideInBytes = sizeof(VertexData);

    // データ転送
    VertexData* vertexData = nullptr;
    data->vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    memcpy(vertexData, data->vertices.data(), sizeof(VertexData) * data->vertices.size());
    data->vertexResource->Unmap(0, nullptr);

    models_[name] = std::move(data);
}

std::shared_ptr<ModelData> ModelManager::GetModelData(const std::string& name) const {
    auto it = models_.find(name);
    if (it != models_.end()) return it->second;
    return nullptr;
}
