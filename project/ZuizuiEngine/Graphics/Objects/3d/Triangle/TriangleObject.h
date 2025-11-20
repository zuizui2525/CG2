#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "Struct.h"
#include "Object3D.h"

class TriangleObject: public Object3D {
public:
    TriangleObject(ID3D12Device* device);
    ~TriangleObject();

    // 更新処理（view, projectionを渡す）
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

    // 描画処理
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        bool enableDraw);
  
private:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
};
