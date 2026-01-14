#pragma once
#include "Struct.h"
#include <memory>
#include <vector>
#include <d3d12.h> 
#include <wrl.h>
#include <random>
#include <list>
#include <string>
#include <algorithm>

// --- 前方宣言 ---
class DxCommon;
class Camera;

struct Particle {
    Transform transform;
    Vector3 velocity;
    Vector4 color;
    float lifeTime;
    float currentTime;
};

struct Emitter {
    Transform transform;
    uint32_t count;
    float frequency;
    float frequencyTime;
};

struct AcclerationField {
    Vector3 acceleration;
    AABB area;
};

class ParticleManager {
public:
    ParticleManager(DxCommon* dxCommon);
    ~ParticleManager() = default;

    void Update(const Camera* camera);
    void Draw(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
        D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
        ID3D12PipelineState* pipelineState,
        ID3D12RootSignature* rootSignature,
        bool draw = true);

    void ImGuiControl(const std::string& name);

    // getter
    ID3D12Resource* GetInstanceResource() const { return instanceResource_.Get(); }
    const Vector3& GetPosition() const { return emitter_.transform.translate; }
    uint32_t        GetCount() const { return emitter_.count; }
    float           GetFrequency() const { return emitter_.frequency; }
    uint32_t        GetMaxInstance() const { return numMaxInstance_; }

    // setter
    void SetPosition(const Vector3& position) { emitter_.transform.translate = position; }
    void SetCount(uint32_t count) { emitter_.count = count; }
    void SetFrequency(float frequency) { emitter_.frequency = frequency; }
    void SetMaxInstance(uint32_t maxInstance);

private:
    // 内部メソッド
    void CreateInstanceResource();
    Particle MakeNewParticle(std::mt19937& randomEngine, Vector3 initialPosition);
    std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);
    void ImGuiSRTControl(const std::string& name);
    void ImGuiParticleControl(const std::string& name);

    // メンバ変数
    DxCommon* dxCommon_ = nullptr;
    Emitter emitter_{};
    AcclerationField accelerationFeild_;

    // 数値管理
    static const UINT kNumMaxInstance = 10000;
    UINT numMaxInstance_ = 0;
    uint32_t numInstance_ = 0;

    // DX12リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    std::vector<VertexData> vertices_;
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
    ParticleForGPU* instanceData_ = nullptr;

    // SRV
    static inline UINT kDescriptorIndex = 50;
    D3D12_CPU_DESCRIPTOR_HANDLE instanceSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU_{};

    // パーティクル・ランダム
    std::list<Particle> particles_;
    std::random_device seedGenerator_;
    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> distribution_{ -20.0f, 20.0f };
    std::uniform_real_distribution<float> distColor_{ 0.0f, 1.0f };
    std::uniform_real_distribution<float> distTime_{ 1.0f, 10.0f };
    const float kDeltaTime_ = 1.0f / 60.0f;
    float alpha_ = 0.0f;

    // 各種フラグ
    bool billboardActive_ = true;
    bool windActive_ = false;
    bool loopActive_ = false;
    bool emitterActive_ = true;
};
