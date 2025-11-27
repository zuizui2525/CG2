#pragma once
#include "Struct.h"
#include <memory>
#include <vector>
#include <d3d12.h> 
#include <wrl.h>
#include <random>
#include <list>
#include <string>

struct Particle {
    Transform transform;
    Vector3 velocity;
    Vector4 color;
    float lifeTime;
    float currentTime;
};

class DxCommon;
class Camera;

class ParticleManager {
public:
    static const UINT kNumMaxInstance = 10;
    uint32_t numInstance_ = 0;

    ParticleManager(DxCommon* dxCommon, const Vector3& initialPosition);
    ~ParticleManager() = default;

    void Update(const Camera* camera);

    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool draw = true);

    void ImGuiControl(const std::string& name);

    ID3D12Resource* GetInstanceResource() const { return instanceResource_.Get(); }

private:
    Particle MakeNewParticle(std::mt19937& randomEngine, Vector3 initialPosition);

    void ImGuiSRTControl(const std::string& name);
    void ImGuiParticleControl(const std::string& name);

    // マネージャ全体のTransform
    Transform transform_{};

    // マテリアルリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    static inline UINT kDescriptorIndex = 50;

    // 頂点バッファ関連
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    std::vector<VertexData> vertices_;

    // インスタンスデータ関連
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
    ParticleForGPU* instanceData_ = nullptr;

    // 個々のパーティクル
    std::list<Particle> particles_;

    // SRVハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE instanceSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU_{};

    // ランダム
    std::random_device seedGenerator_;
    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> distribution_{ -0.5f, 0.5f };
    std::uniform_real_distribution<float> distColor_{ 0.0f,1.0f };
    float alpha_ = 0.0f;
    std::uniform_real_distribution<float> distTime_{ 2.0f,4.0f };
    const float kDeltaTime_ = 1.0f / 60.0f;
    bool billboardActive_ = true;
    bool loopActive_ = true;
};
