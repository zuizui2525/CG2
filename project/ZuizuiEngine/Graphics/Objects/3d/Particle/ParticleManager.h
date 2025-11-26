#pragma once
#include "Object3D.h"
#include "Struct.h"
#include <memory>
#include <vector>
#include <d3d12.h> 
#include <wrl.h>
#include <random>

struct Particle {
    Transform transform;
    Vector3 velocity;
};

class DxCommon;

class ParticleManager : public Object3D {
public:
    static const UINT kNumInstance = 10;

    // 内部で管理するSRVヒープの固定インデックス
    static const UINT kDescriptorIndex = 50;

    // コンストラクタ: DxCommon* と初期位置のみを受け取る
    ParticleManager(DxCommon* dxCommon, const Vector3& initialPosition);
    ~ParticleManager() = default;

    // 毎フレーム更新
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

    // モデル描画用関数
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool draw = true);

    // ImGui
    void ImGuiParticleControl();

    // アクセサ（VBVはDrawでも使用するため公開しても良いが、ここではUpdate/Drawで完結）
    ID3D12Resource* GetInstanceResource() const { return instanceResource_.Get(); }

private:
    Particle MakeNewParticle(std::mt19937& randomEngine);

    // --- 頂点バッファ関連 (クアッド) ---
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    std::vector<VertexData> vertices_;

    // --- インスタンスデータ関連 ---
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
    TransformationMatrix* instanceData_ = nullptr; // Mapされたインスタンスデータへのポインタ

    // CPU側で管理するインスタンスごとのParticle情報
    Particle particles_[kNumInstance];

    // --- SRVハンドル ---
    // SRVヒープ上のCPU/GPUハンドルを保持 (TextureManager方式)
    D3D12_CPU_DESCRIPTOR_HANDLE instanceSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU_{};

    // ランダム
    std::random_device seedGenerator_;
    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> distribution_{ -0.01f, 0.01f };
};
