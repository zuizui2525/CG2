#include "Engine/Graphics/Objects/Effect/ParticleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Math/Collision/Collision.h"
#include <stdexcept>
#include <array>
#include <cassert>
#include "imgui.h" 

namespace {
    void InitializeTransform(Transform& transform) {
        transform.scale = { 1.0f, 1.0f, 1.0f };
        transform.rotate = { 0.0f, 0.0f, 0.0f };
        transform.translate = { 0.0f, 0.0f, 0.0f };
    }
}

void ParticleObject::SetMaxInstance(uint32_t maxInstance) {
    // 値が変わっていない、かつ初期化済みなら何もしない
    if (numMaxInstance_ == maxInstance && isInitialized_) {
        return;
    }

    numMaxInstance_ = (std::min)(maxInstance, (uint32_t)kNumMaxInstance);

    // 最初の1回だけインデックスを決定する（例: staticで管理している場合）
    if (!isInitialized_) {
        static UINT nextIndex = 50;
        mySrvIndex_ = nextIndex++;
        isInitialized_ = true;
    }

    CreateInstanceResource();
}

void ParticleObject::CreateInstanceResource() {
    ID3D12Device* device = sEngine->GetDevice();
    ID3D12DescriptorHeap* srvHeap = sEngine->GetDxCommon()->GetSrvHeap();
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 1. インスタンスデータ用リソースの作成
    // numMaxInstance_ が変化したときだけここに来る
    size_t bufferSize = sizeof(ParticleForGPU) * numMaxInstance_;
    instanceResource_ = DxUtils::CreateBufferResource(device, bufferSize);
    if (!instanceResource_) throw std::runtime_error("Failed to create instanceResource_");

    HRESULT hr = instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
    if (FAILED(hr)) throw std::runtime_error("Failed to map instanceResource_");

    // 2. 固定された mySrvIndex_ を使用してハンドルを取得
    // これにより、kDescriptorIndex が無限に増えるのを防ぎます
    instanceSrvHandleCPU_ = DxUtils::GetCPUDescriptorHandle(srvHeap, descriptorSize, mySrvIndex_);
    instanceSrvHandleGPU_ = DxUtils::GetGPUDescriptorHandle(srvHeap, descriptorSize, mySrvIndex_);

    // 3. SRVの設定
    D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
    instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingSrvDesc.Buffer.FirstElement = 0;
    instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    instancingSrvDesc.Buffer.NumElements = numMaxInstance_; // 新しい最大数で作成
    instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

    // 指定した固定位置（mySrvIndex_）にSRVを書き込む
    device->CreateShaderResourceView(
        instanceResource_.Get(),
        &instancingSrvDesc,
        instanceSrvHandleCPU_
    );
}

void ParticleObject::Initialize(int lightingMode) {
    ID3D12Device* device = sEngine->GetDevice();

    // --- Emitterの初期化 ---
    InitializeTransform(emitter_.transform);
    emitter_.count = 10;
    emitter_.frequency = 0.5f;
    emitter_.frequencyTime = 0.0f;

    // --- 風（Acceleration Field）の初期化 ---
    accelerationFeild_.acceleration = { 50.0f, 0.0f, 0.0f };
    accelerationFeild_.area.min = { -10.0f, -10.0f, 10.0f };
    accelerationFeild_.area.max = { 10.0f, 10.0f, 30.0f };

    // --- Materialリソースの作成 ---
    materialResource_ = DxUtils::CreateBufferResource(device, sizeof(Material));
    if (!materialResource_) throw std::runtime_error("Failed to create materialResource_");

    HRESULT hr_mat = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    if (FAILED(hr_mat) || !materialData_) throw std::runtime_error("Failed to map materialResource_");

    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = lightingMode;
    materialData_->uvtransform = Math::MakeIdentity();

    // --- 頂点バッファ（板ポリゴン）の作成 ---
    vertices_ = {
        // Triangle 1
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        // Triangle 2
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}
    };

    size_t vertexBufferSize = sizeof(VertexData) * vertices_.size();
    vertexResource_ = DxUtils::CreateBufferResource(device, vertexBufferSize);
    if (!vertexResource_) throw std::runtime_error("Failed to create vertexResource_");

    VertexData* vertexData = nullptr;
    HRESULT map_hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    if (FAILED(map_hr)) throw std::runtime_error("Failed to map vertexResource_");
    memcpy(vertexData, vertices_.data(), vertexBufferSize);

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = (UINT)vertexBufferSize;
    vbv_.StrideInBytes = sizeof(VertexData);

    // --- 乱数エンジンの初期化 ---
    randomEngine_ = std::mt19937(seedGenerator_());

    // --- インスタンスリソースの初期確保 (デフォルト100) ---
    SetMaxInstance(100);
}

void ParticleObject::Update() {
    numInstance_ = 0;

    Matrix4x4 managerWorldMatrix = Math::MakeAffineMatrix(
        emitter_.transform.scale,
        emitter_.transform.rotate,
        emitter_.transform.translate
    );

    Matrix4x4 billBoardMatrix = Math::MakeIdentity();
    if (setting_.isBillboard) {
        billBoardMatrix = Math::Inverse(CameraResource::GetCameraManager()->GetViewMatrix3D());
        billBoardMatrix.m[3][0] = 0.0f;
        billBoardMatrix.m[3][1] = 0.0f;
        billBoardMatrix.m[3][2] = 0.0f;
    }

    for (auto particleIterator = particles_.begin(); particleIterator != particles_.end();) {
        if ((*particleIterator).currentTime >= (*particleIterator).lifeTime) {
            particleIterator = particles_.erase(particleIterator);
            continue;
        }

        Matrix4x4 particleWorldMatrix;
        if (setting_.isBillboard) {
            Matrix4x4 scaleMatrix = Math::MakeScaleMatrix((*particleIterator).transform.scale);
            Matrix4x4 rotateMatrix = Math::MakeRotateMatrix((*particleIterator).transform.rotate.x, (*particleIterator).transform.rotate.y, (*particleIterator).transform.rotate.z);
            Matrix4x4 translateMatrix = Math::MakeTranslateMatrix((*particleIterator).transform.translate);
            
            // スケール -> ローカル回転 -> ビルボード（カメラ逆回転） -> 平行移動 の順で掛ける
            particleWorldMatrix = Math::Multiply(scaleMatrix, rotateMatrix);
            particleWorldMatrix = Math::Multiply(particleWorldMatrix, billBoardMatrix);
            particleWorldMatrix = Math::Multiply(particleWorldMatrix, translateMatrix);
        } else {
            particleWorldMatrix = Math::MakeAffineMatrix(
                (*particleIterator).transform.scale,
                (*particleIterator).transform.rotate,
                (*particleIterator).transform.translate
            );
        }

        // 最後にエミッター自体のワールド行列を掛ける
        particleWorldMatrix = Math::Multiply(particleWorldMatrix, managerWorldMatrix);

        Matrix4x4 worldViewProjection = Math::Multiply(particleWorldMatrix, Math::Multiply(CameraResource::GetCameraManager()->GetViewMatrix3D(), CameraResource::GetCameraManager()->GetProjectionMatrix3D()));
        Matrix4x4 worldMatrix = particleWorldMatrix;

        // Fieldの範囲内のParticleには加速度を適応する
        if (IsCollision(accelerationFeild_.area, (*particleIterator).transform.translate) && windActive_) {
            (*particleIterator).velocity += accelerationFeild_.acceleration * kDeltaTime_;
        }

        (*particleIterator).transform.translate += (*particleIterator).velocity * kDeltaTime_;
        (*particleIterator).currentTime += kDeltaTime_;
        
        // 経過割合による線形補間（Lerp）
        float progress = (std::min)((*particleIterator).currentTime / (*particleIterator).lifeTime, 1.0f);
        (*particleIterator).transform.scale = (*particleIterator).startScale + ((*particleIterator).endScale - (*particleIterator).startScale) * progress;
        
        // Vector4の演算子オーバーロードがないため要素ごとに計算
        (*particleIterator).color.x = (*particleIterator).startColor.x + ((*particleIterator).endColor.x - (*particleIterator).startColor.x) * progress;
        (*particleIterator).color.y = (*particleIterator).startColor.y + ((*particleIterator).endColor.y - (*particleIterator).startColor.y) * progress;
        (*particleIterator).color.z = (*particleIterator).startColor.z + ((*particleIterator).endColor.z - (*particleIterator).startColor.z) * progress;
        (*particleIterator).color.w = (*particleIterator).startColor.w + ((*particleIterator).endColor.w - (*particleIterator).startColor.w) * progress;

        if (numInstance_ < numMaxInstance_) {
            instanceData_[numInstance_].WVP = worldViewProjection;
            instanceData_[numInstance_].world = worldMatrix;
            instanceData_[numInstance_].color = (*particleIterator).color;
            ++numInstance_;
        }
        ++particleIterator;
    }

    // エミッターモード（自動発生）
    if (setting_.isEmitter) {
        emitter_.frequencyTime += kDeltaTime_;
        if (setting_.emitFrequency <= emitter_.frequencyTime) {
            size_t currentParticleCount = particles_.size();
            size_t maxEmitCount = (numMaxInstance_ > currentParticleCount) ? (numMaxInstance_ - currentParticleCount) : 0;

            uint32_t emitCount = (std::min)(setting_.emitCountMax, (uint32_t)maxEmitCount);
            if (emitCount > 0) {
                Emitter actualEmitter = emitter_;
                actualEmitter.count = emitCount;
                particles_.splice(particles_.end(), Emit(actualEmitter, randomEngine_));
            }
            emitter_.frequencyTime -= setting_.emitFrequency;
        }
    }
}

void ParticleObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw) return;
    // コマンドリスト
    auto commandList = EngineResource::GetEngine()->GetDxCommon()->GetCommandList();
    // パイプラインの選択
    commandList->SetGraphicsRootSignature(EngineResource::GetEngine()->GetPSOManager()->GetRootSignature("Particle"));
    commandList->SetPipelineState(EngineResource::GetEngine()->GetPSOManager()->GetPSO("Particle"));
    // VBV設定
    commandList->IASetVertexBuffers(0, 1, &vbv_);
    // 定数バッファ設定
    commandList->SetGraphicsRootDescriptorTable(0, instanceSrvHandleGPU_);
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    auto lightMgr = LightResource::GetLightManager();
    if (lightMgr) {
        commandList->SetGraphicsRootConstantBufferView(2, lightMgr->GetDirectionalLightGroupAddress());
    }
    // 指定されたキーでテクスチャ取得
    commandList->SetGraphicsRootDescriptorTable(3, sTexMgr->GetGpuHandle(textureKey));
    // DrawInstanced
    if (draw && numInstance_ > 0) {
        UINT vertexCount = (UINT)vertices_.size();
        commandList->DrawInstanced(vertexCount, numInstance_, 0, 0);
    }
}

void ParticleObject::ImGuiControl(const std::string& name) {
#ifdef _USEIMGUI
    ImGui::Begin("Particle List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            ImGuiSRTControl(name);
            ImGuiParticleControl(name);

        }
        ImGui::End();
    }
#endif
}

void ParticleObject::ImGuiSRTControl(const std::string& name) {
#ifdef _USEIMGUI
    std::string label = "##" + name;

    if (ImGui::CollapsingHeader(("Emitter Transform" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3(("scale" + label).c_str(), &emitter_.transform.scale.x, 0.01f, 0.0f, 0.0f, "%.1f");
        ImGui::DragFloat3(("rotate" + label).c_str(), &emitter_.transform.rotate.x, 0.01f, 0.0f, 0.0f, "%.1f");
        ImGui::DragFloat3(("Translate" + label).c_str(), &emitter_.transform.translate.x, 0.01f, 0.0f, 0.0f, "%.1f");
    }

    if (ImGui::CollapsingHeader(("Material" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit4(("Base Color" + label).c_str(), &materialData_->color.x, ImGuiColorEditFlags_AlphaBar);
    }
#endif
}

void ParticleObject::ImGuiParticleControl(const std::string& name) {
#ifdef _USEIMGUI
    std::string label = "##" + name;
    if (ImGui::CollapsingHeader(("Particle System" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Active Particles: %d / %d", numInstance_, numMaxInstance_);

        float min_dist = distribution_.a();
        float max_dist = distribution_.b();
        float min_time = distTime_.a();
        float max_time = distTime_.b();

        ImGui::SeparatorText("Random Ranges");
        if (ImGui::DragFloat(("Velocity Min" + label).c_str(), &min_dist, 0.1f, 0.0f, 0.0f, "%.1f")) {
            distribution_ = std::uniform_real_distribution<float>(min_dist, max_dist);
        }
        if (ImGui::DragFloat(("Velocity Max" + label).c_str(), &max_dist, 0.1f, 0.0f, 0.0f, "%.1f")) {
            distribution_ = std::uniform_real_distribution<float>(min_dist, max_dist);
        }
        if (ImGui::DragFloat(("Life Min" + label).c_str(), &min_time, 0.1f, 0.0f, 0.0f, "%.1f")) {
            distTime_ = std::uniform_real_distribution<float>(min_time, max_time);
        }
        if (ImGui::DragFloat(("Life Max" + label).c_str(), &max_time, 0.1f, 0.0f, 0.0f, "%.1f")) {
            distTime_ = std::uniform_real_distribution<float>(min_time, max_time);
        }

        ImGui::SeparatorText("Emitter Settings");
        int maxInst = static_cast<int>(numMaxInstance_);
        if (ImGui::DragInt(("Max Instance" + label).c_str(), &maxInst, 1, 1, kNumMaxInstance, "%d")) {
            SetMaxInstance(static_cast<uint32_t>(maxInst));
        }

        int count = static_cast<int>(emitter_.count);
        if (ImGui::DragInt(("Emit Count" + label).c_str(), &count, 1, 1, numMaxInstance_, "%d")) {
            emitter_.count = static_cast<uint32_t>(count);
        }
        ImGui::DragFloat(("Frequency" + label).c_str(), &emitter_.frequency, 0.01f, 0.01f, 10.0f, "%.1f");

        if (ImGui::Button(("Force Spawn 1 Particle" + label).c_str())) {
            if (numInstance_ < numMaxInstance_) {
                particles_.push_back(MakeNewParticle(randomEngine_, emitter_.transform.translate));
            }
        }

        ImGui::SeparatorText("Features");
        ImGui::Checkbox(("Wind (Field)" + label).c_str(), &windActive_);

        if (windActive_) {
            ImGui::Indent();
            ImGui::DragFloat3(("Field Min" + label).c_str(), &accelerationFeild_.area.min.x, 0.1f, 0.0f, 0.0f, "%.1f");
            ImGui::DragFloat3(("Field Max" + label).c_str(), &accelerationFeild_.area.max.x, 0.1f, 0.0f, 0.0f, "%.1f");
            ImGui::DragFloat3(("Acceleration" + label).c_str(), &accelerationFeild_.acceleration.x, 0.1f, 0.0f, 0.0f, "%.1f");
            ImGui::Unindent();
        }
    }
#endif
}

Particle ParticleObject::MakeNewParticle(std::mt19937& randomEngine, Vector3 startPosition) {
    Particle particle{};

    // Position
    std::uniform_real_distribution<float> distPosX(setting_.spawnAreaMin.x, setting_.spawnAreaMax.x);
    std::uniform_real_distribution<float> distPosY(setting_.spawnAreaMin.y, setting_.spawnAreaMax.y);
    std::uniform_real_distribution<float> distPosZ(setting_.spawnAreaMin.z, setting_.spawnAreaMax.z);
    particle.transform.translate = startPosition + setting_.positionOffset + 
                                   Vector3{ distPosX(randomEngine), distPosY(randomEngine), distPosZ(randomEngine) };

    // Scale Start
    std::uniform_real_distribution<float> distScaleX((std::min)(setting_.scaleMin.x, setting_.scaleMax.x), (std::max)(setting_.scaleMin.x, setting_.scaleMax.x));
    std::uniform_real_distribution<float> distScaleY((std::min)(setting_.scaleMin.y, setting_.scaleMax.y), (std::max)(setting_.scaleMin.y, setting_.scaleMax.y));
    std::uniform_real_distribution<float> distScaleZ((std::min)(setting_.scaleMin.z, setting_.scaleMax.z), (std::max)(setting_.scaleMin.z, setting_.scaleMax.z));
    particle.startScale = { distScaleX(randomEngine), distScaleY(randomEngine), distScaleZ(randomEngine) };

    // Scale End
    std::uniform_real_distribution<float> distScaleEndX((std::min)(setting_.scaleEndMin.x, setting_.scaleEndMax.x), (std::max)(setting_.scaleEndMin.x, setting_.scaleEndMax.x));
    std::uniform_real_distribution<float> distScaleEndY((std::min)(setting_.scaleEndMin.y, setting_.scaleEndMax.y), (std::max)(setting_.scaleEndMin.y, setting_.scaleEndMax.y));
    std::uniform_real_distribution<float> distScaleEndZ((std::min)(setting_.scaleEndMin.z, setting_.scaleEndMax.z), (std::max)(setting_.scaleEndMin.z, setting_.scaleEndMax.z));
    particle.endScale = { distScaleEndX(randomEngine), distScaleEndY(randomEngine), distScaleEndZ(randomEngine) };
    
    particle.transform.scale = particle.startScale;

    // Rotate
    std::uniform_real_distribution<float> distRotX((std::min)(setting_.rotationMin.x, setting_.rotationMax.x), (std::max)(setting_.rotationMin.x, setting_.rotationMax.x));
    std::uniform_real_distribution<float> distRotY((std::min)(setting_.rotationMin.y, setting_.rotationMax.y), (std::max)(setting_.rotationMin.y, setting_.rotationMax.y));
    std::uniform_real_distribution<float> distRotZ((std::min)(setting_.rotationMin.z, setting_.rotationMax.z), (std::max)(setting_.rotationMin.z, setting_.rotationMax.z));
    particle.transform.rotate = { distRotX(randomEngine), distRotY(randomEngine), distRotZ(randomEngine) };

    // Velocity
    std::uniform_real_distribution<float> distVelX((std::min)(setting_.velocityMin.x, setting_.velocityMax.x), (std::max)(setting_.velocityMin.x, setting_.velocityMax.x));
    std::uniform_real_distribution<float> distVelY((std::min)(setting_.velocityMin.y, setting_.velocityMax.y), (std::max)(setting_.velocityMin.y, setting_.velocityMax.y));
    std::uniform_real_distribution<float> distVelZ((std::min)(setting_.velocityMin.z, setting_.velocityMax.z), (std::max)(setting_.velocityMin.z, setting_.velocityMax.z));
    particle.velocity = { distVelX(randomEngine), distVelY(randomEngine), distVelZ(randomEngine) };

    // Color Start
    std::uniform_real_distribution<float> distColR((std::min)(setting_.colorStartMin.x, setting_.colorStartMax.x), (std::max)(setting_.colorStartMin.x, setting_.colorStartMax.x));
    std::uniform_real_distribution<float> distColG((std::min)(setting_.colorStartMin.y, setting_.colorStartMax.y), (std::max)(setting_.colorStartMin.y, setting_.colorStartMax.y));
    std::uniform_real_distribution<float> distColB((std::min)(setting_.colorStartMin.z, setting_.colorStartMax.z), (std::max)(setting_.colorStartMin.z, setting_.colorStartMax.z));
    std::uniform_real_distribution<float> distColA((std::min)(setting_.colorStartMin.w, setting_.colorStartMax.w), (std::max)(setting_.colorStartMin.w, setting_.colorStartMax.w));
    particle.startColor = { distColR(randomEngine), distColG(randomEngine), distColB(randomEngine), distColA(randomEngine) };

    // Color End
    std::uniform_real_distribution<float> distColEndR((std::min)(setting_.colorEndMin.x, setting_.colorEndMax.x), (std::max)(setting_.colorEndMin.x, setting_.colorEndMax.x));
    std::uniform_real_distribution<float> distColEndG((std::min)(setting_.colorEndMin.y, setting_.colorEndMax.y), (std::max)(setting_.colorEndMin.y, setting_.colorEndMax.y));
    std::uniform_real_distribution<float> distColEndB((std::min)(setting_.colorEndMin.z, setting_.colorEndMax.z), (std::max)(setting_.colorEndMin.z, setting_.colorEndMax.z));
    std::uniform_real_distribution<float> distColEndA((std::min)(setting_.colorEndMin.w, setting_.colorEndMax.w), (std::max)(setting_.colorEndMin.w, setting_.colorEndMax.w));
    particle.endColor = { distColEndR(randomEngine), distColEndG(randomEngine), distColEndB(randomEngine), distColEndA(randomEngine) };

    particle.color = particle.startColor;

    // LifeTime
    std::uniform_real_distribution<float> distLife((std::min)(setting_.lifeTimeMin, setting_.lifeTimeMax), (std::max)(setting_.lifeTimeMin, setting_.lifeTimeMax));
    particle.lifeTime = distLife(randomEngine);
    particle.currentTime = 0.0f;

    return particle;
}

std::list<Particle> ParticleObject::Emit(const Emitter& emitter, std::mt19937& randomEngine) {
    std::list<Particle> particles;
    for (uint32_t i = 0; i < emitter.count; ++i) {
        particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
    }
    return particles;
}

void ParticleObject::EmitAt(const Vector3& position, uint32_t count) {
    size_t currentParticleCount = particles_.size();
    size_t maxEmitCount = (numMaxInstance_ > currentParticleCount) ? (numMaxInstance_ - currentParticleCount) : 0;
    uint32_t emitCount = (std::min)(count, (uint32_t)maxEmitCount);

    for (uint32_t i = 0; i < emitCount; ++i) {
        particles_.push_back(MakeNewParticle(randomEngine_, position));
    }
}
