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
    Transform transform;  // パーティクルのtransform
    Vector3 velocity;     // 速度
    Vector4 color;        // 色
    float lifeTime;       // 存在時間
    float currentTime;    // 発生してからの経過時間
};

struct Emitter {
    Transform transform;  // エミッタのtransform
    uint32_t count;       // 発生数
    float frequency;      // 発生頻度
    float frequencyTime;  // 頻度用時刻
};

struct AcclerationField {
    Vector3 acceleration; // 加速度
    AABB area;            // 範囲
};

class DxCommon;
class Camera;

class ParticleManager {
public:
    ParticleManager(DxCommon* dxCommon, const Vector3& initialPosition = { 0.0f,0.0f,0.0f }, const int maxInstance = 100, const int count = 10, const float frequency = 0.5);
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
    Vector3& GetPosition() { return emitter_.transform.translate; }
    void SetPosition(const Vector3& position) { emitter_.transform.translate = position; }

private:
    Particle MakeNewParticle(std::mt19937& randomEngine, Vector3 initialPosition);
    std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);
    
    void ImGuiSRTControl(const std::string& name);
    void ImGuiParticleControl(const std::string& name);

    // Emitter
    Emitter emitter_{};

    // 風
    AcclerationField accelerationFeild_;

    // パーティクルの最大発生数
    static const UINT kNumMaxInstance = 10000;
    UINT numMaxInstance_ = 0;
    uint32_t numInstance_ = 0;

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
    std::uniform_real_distribution<float> distribution_{ -20.0f, 20.0f };
    std::uniform_real_distribution<float> distColor_{ 0.0f,1.0f };
    float alpha_ = 0.0f;
    std::uniform_real_distribution<float> distTime_{ 1.0f,10.0f };
    const float kDeltaTime_ = 1.0f / 60.0f;
   
    // フラグ
    bool billboardActive_ = true;  // ビルボード

    bool loopActive_ = false;      // ループするのか
    bool emitterActive_ = true;    // エミッタをつかうのか
};
