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
    // コンストラクタから引数を削除
    SpriteObject() = default;
    ~SpriteObject() = default;

    void Initialize(ID3D12Device* device);
    void Update(const Camera* camera);
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool enableDraw);

    void ImGuiControl();

    // Getter
    Transform& GetTransform() { return transform_; }
    Vector3& GetScale() { return transform_.scale; }
    Vector3& GetRotate() { return transform_.rotate; }
    Vector3& GetPosition() { return transform_.translate; }
    Transform& GetUVTransform() { return uvTransform_; }
    Material* GetMaterialData() { return materialData_; }
    float GetWidth() const { return width_; }
    float GetHeight() const { return height_; }

    // Setter
    void SetTransform(const Transform& transform) { transform_ = transform; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetPosition(const Vector3& position) { transform_.translate = position; }

    // サイズ設定用のSetter
    void SetSize(float width, float height);

private:
    // 頂点座標を更新する内部関数
    void UpdateVertexData();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    Material* materialData_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;
    VertexData* vertexData_ = nullptr; // 頂点データへのポインタを保持

    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    Transform uvTransform_{ {1,1,1}, {0,0,0}, {0,0,0} };

    float width_ = 100.0f;  // デフォルト幅
    float height_ = 100.0f; // デフォルト高さ
};
