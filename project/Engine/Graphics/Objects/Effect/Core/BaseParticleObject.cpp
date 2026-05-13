#include "BaseParticleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include <imgui.h>

namespace {
    void InitializeTransform(Transform& transform) {
        transform.scale = { 1.0f, 1.0f, 1.0f };
        transform.rotate = { 0.0f, 0.0f, 0.0f };
        transform.translate = { 0.0f, 0.0f, 0.0f };
    }
}

void BaseParticleObject::Initialize(int lightingMode) {
    ID3D12Device* device = sEngine->GetDevice();

    InitializeTransform(emitter_.transform);
    emitter_.count = 10;
    emitter_.frequency = 0.5f;
    emitter_.frequencyTime = 0.0f;

    materialResource_ = DxUtils::CreateBufferResource(device, sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = lightingMode;
    materialData_->uvtransform = Math::MakeIdentity();

    randomEngine_ = std::mt19937(seedGenerator_());

    // SRVインデックスの固定割り当て (以前のコードと同じ50から開始)
    static UINT nextIndex = 50; 
    mySrvIndex_ = nextIndex++;
    
    CreateInstanceResource();
}

void BaseParticleObject::CreateInstanceResource() {
    ID3D12Device* device = sEngine->GetDevice();
    ID3D12DescriptorHeap* srvHeap = sEngine->GetDxCommon()->GetSrvHeap();
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    size_t bufferSize = sizeof(ParticleForGPU) * kNumMaxInstance;
    instanceResource_ = DxUtils::CreateBufferResource(device, bufferSize);
    instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));

    instanceSrvHandleCPU_ = DxUtils::GetCPUDescriptorHandle(srvHeap, descriptorSize, mySrvIndex_);
    instanceSrvHandleGPU_ = DxUtils::GetGPUDescriptorHandle(srvHeap, descriptorSize, mySrvIndex_);

    D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
    instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingSrvDesc.Buffer.FirstElement = 0;
    instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
    instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

    device->CreateShaderResourceView(instanceResource_.Get(), &instancingSrvDesc, instanceSrvHandleCPU_);
}

void BaseParticleObject::Update() {
    numInstance_ = 0;

    // エミッター更新
    if (setting_.isEmitter) {
        emitter_.frequencyTime += kDeltaTime_;
        if (setting_.emitFrequency <= emitter_.frequencyTime) {
            std::list<Particle> newParticles = Emit(emitter_, randomEngine_);
            particles_.splice(particles_.end(), newParticles);
            emitter_.frequencyTime -= setting_.emitFrequency; // 以前の引き算方式に戻す
        }
    }

    auto cameraMgr = CameraResource::GetCameraManager();
    Matrix4x4 managerWorldMatrix = Math::MakeAffineMatrix(emitter_.transform.scale, emitter_.transform.rotate, emitter_.transform.translate);

    Matrix4x4 billBoardMatrix = Math::MakeIdentity();
    if (setting_.isBillboard) {
        billBoardMatrix = Math::Inverse(cameraMgr->GetViewMatrix3D());
        billBoardMatrix.m[3][0] = 0.0f;
        billBoardMatrix.m[3][1] = 0.0f;
        billBoardMatrix.m[3][2] = 0.0f;
    }

    for (auto it = particles_.begin(); it != particles_.end(); ) {
        if (it->currentTime >= it->lifeTime) {
            // 消滅する瞬間に onDeathEffectName が設定されていれば発生させる
            if (!setting_.onDeathEffectName.empty()) {
                EffectPlayParam param;
                param.position = it->transform.translate;
                EffectManager::GetInstance()->PlayEffect3D(setting_.onDeathEffectName, param);
            }
            it = particles_.erase(it);
            continue;
        }

        // トレイル（移動中の連鎖エフェクト）の発生処理
        if (!setting_.trailEffectName.empty()) {
            it->trailFrequencyTimer += kDeltaTime_;
            if (it->trailFrequencyTimer >= setting_.trailFrequency) {
                EffectPlayParam param;
                param.position = it->transform.translate;
                EffectManager::GetInstance()->PlayEffect3D(setting_.trailEffectName, param);
                it->trailFrequencyTimer -= setting_.trailFrequency;
            }
        }

        Vector3 currentScale = it->transform.scale;
        Vector3 currentRotate = it->transform.rotate;

        if (setting_.isVelocityAligned) {
            float speed = std::sqrt(it->velocity.x * it->velocity.x + it->velocity.y * it->velocity.y + it->velocity.z * it->velocity.z);
            if (speed > 0.001f) {
                Vector3 dir = { it->velocity.x / speed, it->velocity.y / speed, it->velocity.z / speed };
                currentRotate.y = std::atan2(dir.x, dir.z);
                currentRotate.x = std::asin(-dir.y);
                currentRotate.z = 0.0f;
                // 進行方向（Z軸）をスピードに比例して引き伸ばす（長さ調整用の係数 0.15f）
                currentScale.z = speed * 0.15f; 
                // Y軸・X軸は細くする
                currentScale.x *= 0.2f;
                currentScale.y *= 0.2f;
            }
        }

        Matrix4x4 particleWorldMatrix;
        if (setting_.isBillboard) {
            Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(currentScale);
            Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(currentRotate.x, currentRotate.y, currentRotate.z);
            Matrix4x4 translateMatrix = Math::MakeTranslateMatrix(it->transform.translate);
            
            particleWorldMatrix = Math::Multiply(scaleMatrix, rotateMatrix);
            particleWorldMatrix = Math::Multiply(particleWorldMatrix, billBoardMatrix);
            particleWorldMatrix = Math::Multiply(particleWorldMatrix, translateMatrix);
        } else {
            particleWorldMatrix = Math::MakeAffineMatrix(currentScale, currentRotate, it->transform.translate);
        }

        particleWorldMatrix = Math::Multiply(particleWorldMatrix, managerWorldMatrix);

        Matrix4x4 worldViewProjection = Math::Multiply(particleWorldMatrix, Math::Multiply(cameraMgr->GetViewMatrix3D(), cameraMgr->GetProjectionMatrix3D()));

        // 加速度（重力など）を速度に加算
        it->velocity.x += setting_.acceleration.x * kDeltaTime_;
        it->velocity.y += setting_.acceleration.y * kDeltaTime_;
        it->velocity.z += setting_.acceleration.z * kDeltaTime_;

        // 速度を座標に加算
        it->transform.translate += it->velocity * kDeltaTime_;
        it->currentTime += kDeltaTime_;

        float progress = (std::min)(it->currentTime / it->lifeTime, 1.0f);
        it->transform.scale = it->startScale + (it->endScale - it->startScale) * progress;
        
        it->color.x = it->startColor.x + (it->endColor.x - it->startColor.x) * progress;
        it->color.y = it->startColor.y + (it->endColor.y - it->startColor.y) * progress;
        it->color.z = it->startColor.z + (it->endColor.z - it->startColor.z) * progress;
        it->color.w = it->startColor.w + (it->endColor.w - it->startColor.w) * progress;

        if (numInstance_ < kNumMaxInstance) {
            instanceData_[numInstance_].world = particleWorldMatrix;
            instanceData_[numInstance_].WVP = worldViewProjection;
            instanceData_[numInstance_].color = it->color;
            numInstance_++;
        }
        it++;
    }
}

void BaseParticleObject::EmitAt(uint32_t count, const EffectPlayParam& param) {
    if (!param.textureKey.empty()) {
        setting_.textureName = param.textureKey;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (particles_.size() < kNumMaxInstance) {
            particles_.push_back(MakeNewParticle(randomEngine_, param));
        }
    }
}

Particle BaseParticleObject::MakeNewParticle(std::mt19937& randomEngine, const EffectPlayParam& param) {
    Particle particle;
    
    std::uniform_real_distribution<float> distPosX(setting_.spawnAreaMin.x, setting_.spawnAreaMax.x);
    std::uniform_real_distribution<float> distPosY(setting_.spawnAreaMin.y, setting_.spawnAreaMax.y);
    std::uniform_real_distribution<float> distPosZ(setting_.spawnAreaMin.z, setting_.spawnAreaMax.z);
    
    particle.transform.translate = param.position + setting_.positionOffset + 
                                   Vector3{ distPosX(randomEngine), distPosY(randomEngine), distPosZ(randomEngine) };

    auto getRandVec3 = [&](const Vector3& min, const Vector3& max) -> Vector3 {
        return {
            std::uniform_real_distribution<float>((std::min)(min.x, max.x), (std::max)(min.x, max.x))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.y, max.y), (std::max)(min.y, max.y))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.z, max.z), (std::max)(min.z, max.z))(randomEngine)
        };
    };

    particle.startScale = getRandVec3(setting_.scaleMin, setting_.scaleMax);
    particle.startScale.x *= param.scale.x;
    particle.startScale.y *= param.scale.y;
    particle.startScale.z *= param.scale.z;

    // 未設定（負の値）なら開始時と同じにする
    if (setting_.scaleEndMin.x < 0.0f) {
        particle.endScale = particle.startScale;
    } else {
        particle.endScale = getRandVec3(setting_.scaleEndMin, setting_.scaleEndMax);
        particle.endScale.x *= param.scale.x;
        particle.endScale.y *= param.scale.y;
        particle.endScale.z *= param.scale.z;
    }
    
    particle.transform.scale = particle.startScale;
    particle.transform.rotate = getRandVec3(setting_.rotationMin, setting_.rotationMax);
    particle.transform.rotate.x += param.rotation.x;
    particle.transform.rotate.y += param.rotation.y;
    particle.transform.rotate.z += param.rotation.z;
    
    Vector3 baseVel;
    if (setting_.isSpherical) {
        // 球状拡散：velocityMin.x を最小スピード、velocityMax.x を最大スピードとする
        float speedMin = setting_.velocityMin.x;
        float speedMax = setting_.velocityMax.x;
        float speed = std::uniform_real_distribution<float>((std::min)(speedMin, speedMax), (std::max)(speedMin, speedMax))(randomEngine);
        
        // 極座標系でランダムな方向ベクトルを生成
        float theta = std::uniform_real_distribution<float>(0.0f, 3.14159265f * 2.0f)(randomEngine); // 方位角 (0〜2PI)
        float phi = std::acos(std::uniform_real_distribution<float>(-1.0f, 1.0f)(randomEngine));     // 仰角 (-1〜1の逆余弦で一様な全球分布)
        
        Vector3 direction;
        direction.x = std::sin(phi) * std::cos(theta);
        direction.y = std::sin(phi) * std::sin(theta);
        direction.z = std::cos(phi);
        
        baseVel.x = direction.x * speed;
        baseVel.y = direction.y * speed;
        baseVel.z = direction.z * speed;
    } else {
        baseVel = getRandVec3(setting_.velocityMin, setting_.velocityMax);
    }
    
    particle.velocity = baseVel + param.velocityOverride;

    auto getRandVec4 = [&](const Vector4& min, const Vector4& max) -> Vector4 {
        return {
            std::uniform_real_distribution<float>((std::min)(min.x, max.x), (std::max)(min.x, max.x))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.y, max.y), (std::max)(min.y, max.y))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.z, max.z), (std::max)(min.z, max.z))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.w, max.w), (std::max)(min.w, max.w))(randomEngine)
        };
    };

    particle.startColor = getRandVec4(setting_.colorStartMin, setting_.colorStartMax);
    // 未設定（負の値）なら、RGBは維持してアルファだけ0（フェードアウト）にする
    if (setting_.colorEndMin.x < 0.0f) {
        particle.endColor = particle.startColor;
        particle.endColor.w = 0.0f; 
    } else {
        particle.endColor = getRandVec4(setting_.colorEndMin, setting_.colorEndMax);
    }
    particle.color = particle.startColor;

    particle.lifeTime = std::uniform_real_distribution<float>((std::min)(setting_.lifeTimeMin, setting_.lifeTimeMax), (std::max)(setting_.lifeTimeMin, setting_.lifeTimeMax))(randomEngine);
    particle.currentTime = 0.0f;

    return particle;
}

std::list<Particle> BaseParticleObject::Emit(const Emitter& emitter, std::mt19937& randomEngine) {
    std::list<Particle> particles;
    EffectPlayParam param;
    param.position = emitter.transform.translate;
    param.rotation = emitter.transform.rotate;
    param.scale = emitter.transform.scale;
    uint32_t count = std::uniform_int_distribution<uint32_t>(setting_.emitCountMin, setting_.emitCountMax)(randomEngine);
    for (uint32_t i = 0; i < count; ++i) {
        particles.push_back(MakeNewParticle(randomEngine, param));
    }
    return particles;
}

void BaseParticleObject::ImGuiControl(const std::string& name) {
    // 省略 (必要に応じて後で実装)
}
