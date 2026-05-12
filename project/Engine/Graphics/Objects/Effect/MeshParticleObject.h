#pragma once
#include "BaseParticleObject.h"

class MeshParticleObject : public BaseParticleObject {
public:
    void Initialize(int lightingMode = 2) override; // 3Dなのでデフォルトでライティング有効
    void Draw(const std::string& textureKey = "white", bool draw = true) override;

private:
    void CreateCubeMesh();

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    D3D12_INDEX_BUFFER_VIEW ibv_{};
    uint32_t indexCount_ = 0;
};
