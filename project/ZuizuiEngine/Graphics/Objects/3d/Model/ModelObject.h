#pragma once
#include "Object3D.h"
#include "Struct.h"
#include <memory>
#include <string>
#include <vector>

// objモデル用クラス
class ModelObject : public Object3D {
public:
    ModelObject(ID3D12Device* device,
        const std::string& filename,
        const Vector3& initialPosition = { 0.0f, 0.0f, 0.0f }); // ★ 初期位置追加

    // 頂点バッファビュー取得
    D3D12_VERTEX_BUFFER_VIEW GetVBV() const { return vbv_; }

    // モデルデータ取得（テクスチャパスなどを参照する用）
    const std::shared_ptr<ModelData>& GetModelData() const { return modelData_; }

    // 毎フレーム更新（Transform から行列計算して WVP 反映）
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

    // モデル描画用関数
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool draw = true);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    std::shared_ptr<ModelData> modelData_;
};
