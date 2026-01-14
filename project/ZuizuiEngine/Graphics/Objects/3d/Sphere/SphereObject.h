#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include "Struct.h"
#include "Object3D.h"

class Camera;

class SphereObject : public Object3D {
public:
    // 初期値を 16, 1.0f に設定
    SphereObject(ID3D12Device* device);
    ~SphereObject();

    // 更新処理 (パラメータ変更があればメッシュを再生成する)
    void Update(const Camera* camera);

    // 描画処理
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool enableDraw);

    // Getter
    float GetRadius() const { return radius_; }
    uint32_t GetSubdivision() const { return subdivision_; }

    // Setter (変更があったらフラグを立てる)
    void SetRadius(float radius) { radius_ = radius; needsUpdate_ = true; }
    void SetSubdivision(uint32_t subdivision) { subdivision_ = subdivision; needsUpdate_ = true; }

private:
    // メッシュ（頂点・インデックスバッファ）の生成
    void CreateMesh();

private:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    ID3D12Device* device_ = nullptr;

    // パラメータ
    uint32_t subdivision_ = 16;
    float radius_ = 1.0f;
    bool needsUpdate_ = false; // 再生成フラグ
};
