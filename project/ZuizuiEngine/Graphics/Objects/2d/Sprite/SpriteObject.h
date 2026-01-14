#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "Matrix.h"
#include "Struct.h"
#include "Function.h"

class Camera;

class SpriteObject {
public:
    SpriteObject(ID3D12Device* device, int width, int height);
    ~SpriteObject();

    // 更新（ModelObjectと揃える）
    void Update(const Camera* camera);

    // 描画
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool enableDraw);

    // ImGui
    void ImGuiControl();

    // Getter
    Transform& GetTransform() { return transform_; }
    Vector3& GetScale() { return transform_.scale; }
    Vector3& GetRotate() { return transform_.rotate; }
    Vector3& GetPosition() { return transform_.translate; }
    Transform& GetUVTransform() { return uvTransform_; }
    Material* GetMaterialData() { return materialData_; }

    // Setter
    void SetTransform(Transform& transform) { transform_ = transform; }
    void SetScale(Vector3& scale) { transform_.scale = scale; }
    void SetRotate(Vector3& rotate) { transform_.rotate = rotate; }
    void SetPosition(Vector3& position) { transform_.translate = position; }

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
