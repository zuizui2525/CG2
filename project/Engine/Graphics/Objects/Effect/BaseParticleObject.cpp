#include "BaseParticleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
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
            it = particles_.erase(it);
            continue;
        }

        Matrix4x4 particleWorldMatrix;
        if (setting_.isBillboard) {
            Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(it->transform.scale);
            Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(it->transform.rotate.x, it->transform.rotate.y, it->transform.rotate.z);
            Matrix4x4 translateMatrix = Math::MakeTranslateMatrix(it->transform.translate);
            
            particleWorldMatrix = Math::Multiply(scaleMatrix, rotateMatrix);
            particleWorldMatrix = Math::Multiply(particleWorldMatrix, billBoardMatrix);
            particleWorldMatrix = Math::Multiply(particleWorldMatrix, translateMatrix);
        } else {
            particleWorldMatrix = Math::MakeAffineMatrix(it->transform.scale, it->transform.rotate, it->transform.translate);
        }

        particleWorldMatrix = Math::Multiply(particleWorldMatrix, managerWorldMatrix);

        Matrix4x4 worldViewProjection = Math::Multiply(particleWorldMatrix, Math::Multiply(cameraMgr->GetViewMatrix3D(), cameraMgr->GetProjectionMatrix3D()));

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

void BaseParticleObject::EmitAt(const Vector3& position, uint32_t count, const Vector3& velocityOverride, const std::string& textureOverride) {
    if (!textureOverride.empty()) {
        setting_.textureName = textureOverride;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (particles_.size() < kNumMaxInstance) {
            particles_.push_back(MakeNewParticle(randomEngine_, position, velocityOverride));
        }
    }
}

Particle BaseParticleObject::MakeNewParticle(std::mt19937& randomEngine, Vector3 initialPosition, const Vector3& velocityOverride) {
    Particle particle;
    
    std::uniform_real_distribution<float> distPosX(setting_.spawnAreaMin.x, setting_.spawnAreaMax.x);
    std::uniform_real_distribution<float> distPosY(setting_.spawnAreaMin.y, setting_.spawnAreaMax.y);
    std::uniform_real_distribution<float> distPosZ(setting_.spawnAreaMin.z, setting_.spawnAreaMax.z);
    
    particle.transform.translate = initialPosition + setting_.positionOffset + 
                                   Vector3{ distPosX(randomEngine), distPosY(randomEngine), distPosZ(randomEngine) };

    auto getRandVec3 = [&](const Vector3& min, const Vector3& max) -> Vector3 {
        return {
            std::uniform_real_distribution<float>((std::min)(min.x, max.x), (std::max)(min.x, max.x))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.y, max.y), (std::max)(min.y, max.y))(randomEngine),
            std::uniform_real_distribution<float>((std::min)(min.z, max.z), (std::max)(min.z, max.z))(randomEngine)
        };
    };

    particle.startScale = getRandVec3(setting_.scaleMin, setting_.scaleMax);
    // 未設定（負の値）なら開始時と同じにする
    if (setting_.scaleEndMin.x < 0.0f) {
        particle.endScale = particle.startScale;
    } else {
        particle.endScale = getRandVec3(setting_.scaleEndMin, setting_.scaleEndMax);
    }
    
    particle.transform.scale = particle.startScale;
    particle.transform.rotate = getRandVec3(setting_.rotationMin, setting_.rotationMax);
    
    Vector3 baseVel = getRandVec3(setting_.velocityMin, setting_.velocityMax);
    particle.velocity = baseVel + velocityOverride;

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
    uint32_t count = std::uniform_int_distribution<uint32_t>(setting_.emitCountMin, setting_.emitCountMax)(randomEngine);
    for (uint32_t i = 0; i < count; ++i) {
        particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
    }
    return particles;
}

void BaseParticleObject::ImGuiControl(const std::string& name) {
    // 省略 (必要に応じて後で実装)
}
