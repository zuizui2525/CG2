#pragma once
#include "Engine/Math/MathStructs.h"
#include "Engine/Math/Collision/CollisionStructs.h"
#include "Engine/Graphics/RenderStructs.h"
#include "Engine/Base/BaseResource.h"
#include "../Settings/EffectSetting.h"
#include <memory>
#include <vector>
#include <d3d12.h> 
#include <wrl.h>
#include <random>
#include <list>
#include <string>
#include <algorithm>

struct Particle {
    Transform transform;
    Vector3 velocity;
    Vector3 rotationVelocity;
    
    // 時間変化用のパラメータ
    Vector3 startScale;
    Vector3 endScale;
    Vector4 startColor;
    Vector4 endColor;
    
    Vector4 color; // 現在の色
    float lifeTime;
    float currentTime;
    float trailFrequencyTimer = 0.0f; // トレイル（火の粉）発生用の内部タイマー
    Vector4 inheritColor = { 0.0f, 0.0f, 0.0f, 0.0f }; // 追加：子エフェクトに引き継ぐ色
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

// 2D/3Dパーティクルの共通ロジックを管理する基底クラス
class BaseParticleObject : public Base3D {
public:
    virtual ~BaseParticleObject();

    virtual void Initialize(int lightingMode = 0);
    virtual void Update();
    virtual void Draw(const std::string& textureKey = "white", bool draw = true) = 0;

    void ImGuiControl(const std::string& name);

    // 特定の座標でワンショット発生させる
    void EmitAt(uint32_t count, const EffectPlayParam& param);

    // getter/setter
    Transform& GetTransform() { return emitter_.transform; }
    void SetPosition(const Vector3& position) { emitter_.transform.translate = position; }
    void SetEmitterMode(bool active) { setting_.isEmitter = active; }
    bool GetEmitterMode() const { return setting_.isEmitter; }
    void SetSetting(const EffectSetting& setting) { setting_ = setting; }
    EffectSetting& GetSettingRef() { return setting_; }
    const EffectSetting& GetSetting() const { return setting_; }

protected:
    void CreateInstanceResource();
    Particle MakeNewParticle(std::mt19937& randomEngine, const EffectPlayParam& param);
    std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);

    // メンバ変数
    Emitter emitter_{};
    AcclerationField accelerationFeild_;
    EffectSetting setting_{};

    // 数値管理
    static const UINT kNumMaxInstance = 10000;
    UINT numMaxInstance_ = 1000;
    uint32_t numInstance_ = 0;

    // DX12リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
    ParticleForGPU* instanceData_ = nullptr;

    // SRV
    UINT mySrvIndex_ = 0;
    bool isInitialized_ = false;
    D3D12_CPU_DESCRIPTOR_HANDLE instanceSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE instanceSrvHandleGPU_{};

    // パーティクル・ランダム
    std::list<Particle> particles_;
    std::random_device seedGenerator_;
    std::mt19937 randomEngine_;
    const float kDeltaTime_ = 1.0f / 60.0f;

    // ImGuiウィンドウの開閉状態
    bool isWindowOpen_ = false;
};
