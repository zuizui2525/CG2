#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "Matrix.h"
#include "Struct.h"
#include "Function.h"

class TriangleObject {
public:
    TriangleObject(ID3D12Device* device);
    ~TriangleObject();

    // 更新処理（view, projectionを渡す）
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

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};

    // CPUから触る用のポインタ
    Material* materialData_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;

    // Transform
    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    Transform uvTransform_{ {1,1,1}, {0,0,0}, {0,0,0} };
};
