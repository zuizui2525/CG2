#pragma once
#include "BaseParticleObject.h"

class SpriteParticleObject : public BaseParticleObject {
public:
    void Initialize(int lightingMode = 0) override;
    void Draw(const std::string& textureKey = "white", bool draw = true) override;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    std::vector<VertexData> vertices_;
};
