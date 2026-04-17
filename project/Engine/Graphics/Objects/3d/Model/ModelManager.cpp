#include "Engine/Graphics/Objects/3d/Model/ModelManager.h"
#include "Engine/Graphics/Objects/3d/Model/ModelLoader.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include "Engine/Base/BaseResource.h"

void ModelManager::Initialize() {
    // Engine
    auto engine = EngineResource::GetEngine();
    assert(engine != nullptr);
    device_ = engine->GetDevice();

    // TexMgr
    auto texMgr = TextureResource::GetTextureManager();
    assert(texMgr != nullptr);
    texMgr_ = texMgr;
}

void ModelManager::LoadModel(const std::string& name, const std::string& filename) {
    if (models_.find(name) != models_.end()) return;

    auto data = std::make_shared<ModelData>(ModelLoader::LoadObjFile(filename));

    // 頂点リソース作成
    data->vertexResource = DxUtils::CreateBufferResource(device_, sizeof(VertexData) * data->vertices.size());
    data->vbv.BufferLocation = data->vertexResource->GetGPUVirtualAddress();
    data->vbv.SizeInBytes = sizeof(VertexData) * (UINT)data->vertices.size();
    data->vbv.StrideInBytes = sizeof(VertexData);

    // データ転送
    VertexData* vertexData = nullptr;
    data->vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    memcpy(vertexData, data->vertices.data(), sizeof(VertexData) * data->vertices.size());
    data->vertexResource->Unmap(0, nullptr);

    models_[name] = std::move(data);
    texMgr_->LoadTexture(name, models_[name]->material.textureFilePath);
}

std::shared_ptr<ModelData> ModelManager::GetModelData(const std::string& name) const {
    auto it = models_.find(name);
    if (it != models_.end()) return it->second;
    return nullptr;
}
