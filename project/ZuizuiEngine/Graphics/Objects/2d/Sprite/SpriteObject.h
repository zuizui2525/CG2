#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "Matrix.h"
#include "Struct.h"
#include "Function.h"

class SpriteObject {
public:
    SpriteObject(ID3D12Device* device, int width, int height);
    ~SpriteObject();

    // 更新（ModelObjectと揃える）
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

    // 描画
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool enableDraw);

    // ImGui
    void ImGuiControl();

    // getter
    Transform& GetTransform() { return transform_; }
    Transform& GetUVTransform() { return uvTransform_; }
    Material* GetMaterialData() { return materialData_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    Material* materialData_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    Transform uvTransform_{ {1,1,1}, {0,0,0}, {0,0,0} };
};
