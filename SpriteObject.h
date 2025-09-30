#pragma once
#include "Object3D.h"
#include "Struct.h"

// 2Dスプライト用クラス
class SpriteObject : public Object3D {
public:
    // width, height に加えて position も渡せるようにした
    SpriteObject(ID3D12Device* device, float width, float height, const Vector3& position);

    D3D12_VERTEX_BUFFER_VIEW GetVBV() const { return vbv_; }
    D3D12_INDEX_BUFFER_VIEW  GetIBV() const { return ibv_; }

    Material* GetMaterialData() { return materialData_; }

    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
    void UpdateUVTransform();
    void Draw(ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* directionalLightResource,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        bool draw = true);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    D3D12_INDEX_BUFFER_VIEW  ibv_{};

    Material* materialData_ = nullptr;
};
