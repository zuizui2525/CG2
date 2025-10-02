#pragma once
#include "Header.h"

class SphereObject {
public:
    SphereObject(ID3D12Device* device, uint32_t subdivision = 16, float radius = 1.0f);
    ~SphereObject();

    // 更新処理 (view, projectionを渡す)
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

    // 描画処理
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        ID3D12Resource* directionalLightResource,
        bool enableDraw);

    // getter
    Transform& GetTransform() { return transform_; }
    Transform& GetUVTransform() { return uvTransform_; }
    Material* GetMaterialData() { return materialData_; }

private:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    // CPUから触る用のポインタ
    Material* materialData_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;

    // Transform
    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    Transform uvTransform_{ {1,1,1}, {0,0,0}, {0,0,0} };

    // subdivisionと半径
    uint32_t subdivision_ = 16;
    float radius_ = 1.0f;
};
