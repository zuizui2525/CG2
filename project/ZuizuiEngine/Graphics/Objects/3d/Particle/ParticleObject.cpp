#include "ParticleObject.h"
#include "Function.h"
#include "Matrix.h" 
#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "TextureManager.h"
#include "Collision.h"
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
    ID3D12Device* device = engine_->GetDevice();
    ID3D12DescriptorHeap* srvHeap = engine_->GetDxCommon()->GetSrvHeap();
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 1. インスタンスデータ用リソースの作成
    // numMaxInstance_ が変化したときだけここに来る
    size_t bufferSize = sizeof(ParticleForGPU) * numMaxInstance_;
    instanceResource_ = CreateBufferResource(device, bufferSize);
    if (!instanceResource_) throw std::runtime_error("Failed to create instanceResource_");

    HRESULT hr = instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
    if (FAILED(hr)) throw std::runtime_error("Failed to map instanceResource_");

    // 2. 固定された mySrvIndex_ を使用してハンドルを取得
    // これにより、kDescriptorIndex が無限に増えるのを防ぎます
    instanceSrvHandleCPU_ = GetCPUDescriptorHandle(srvHeap, descriptorSize, mySrvIndex_);
    instanceSrvHandleGPU_ = GetGPUDescriptorHandle(srvHeap, descriptorSize, mySrvIndex_);

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

void ParticleObject::Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, TextureManager* texture, int lightingMode) {
    engine_ = engine;
    camera_ = camera;
    dirLight_ = light;
    texture_ = texture;
    ID3D12Device* device = engine_->GetDevice();

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
    materialResource_ = CreateBufferResource(device, sizeof(Material));
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
    vertexResource_ = CreateBufferResource(device, vertexBufferSize);
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
    if (billboardActive_) {
        billBoardMatrix = Math::Inverse(camera_->GetViewMatrix3D());
        billBoardMatrix.m[3][0] = 0.0f;
        billBoardMatrix.m[3][1] = 0.0f;
        billBoardMatrix.m[3][2] = 0.0f;
    }

    for (auto particleIterator = particles_.begin(); particleIterator != particles_.end();) {
        if ((*particleIterator).currentTime >= (*particleIterator).lifeTime) {
            particleIterator = particles_.erase(particleIterator);
            continue;
        }

        Matrix4x4 particleWorldMatrix = Math::MakeAffineMatrix(
            (*particleIterator).transform.scale,
            (*particleIterator).transform.rotate,
            (*particleIterator).transform.translate
        );

        particleWorldMatrix = Math::Multiply(billBoardMatrix, particleWorldMatrix);
        particleWorldMatrix = Math::Multiply(particleWorldMatrix, managerWorldMatrix);

        Matrix4x4 worldViewProjection = Math::Multiply(particleWorldMatrix, Math::Multiply(camera_->GetViewMatrix3D(), camera_->GetProjectionMatrix3D()));
        Matrix4x4 worldMatrix = particleWorldMatrix;

        // Fieldの範囲内のParticleには加速度を適応する
        if (IsCollision(accelerationFeild_.area, (*particleIterator).transform.translate) && windActive_) {
            (*particleIterator).velocity += accelerationFeild_.acceleration * kDeltaTime_;
        }

        (*particleIterator).transform.translate += (*particleIterator).velocity * kDeltaTime_;
        (*particleIterator).currentTime += kDeltaTime_;
        alpha_ = 1.0f - ((*particleIterator).currentTime / (*particleIterator).lifeTime);

        if (numInstance_ < numMaxInstance_) {
            instanceData_[numInstance_].WVP = worldViewProjection;
            instanceData_[numInstance_].world = worldMatrix;
            instanceData_[numInstance_].color = (*particleIterator).color;
            instanceData_[numInstance_].color.w = alpha_;
            ++numInstance_;
        }
        ++particleIterator;
    }

    if (loopActive_ && !emitterActive_) {
        size_t currentParticleCount = particles_.size();
        if (currentParticleCount < numMaxInstance_) {
            size_t neededCount = numMaxInstance_ - currentParticleCount;
            for (size_t i = 0; i < neededCount; ++i) {
                particles_.push_back(MakeNewParticle(randomEngine_, emitter_.transform.translate));
            }
        }
    }

    if (emitterActive_ && !loopActive_) {
        emitter_.frequencyTime += kDeltaTime_;
        if (emitter_.frequency <= emitter_.frequencyTime) {
            size_t currentParticleCount = particles_.size();
            size_t maxEmitCount = (numMaxInstance_ > currentParticleCount) ? (numMaxInstance_ - currentParticleCount) : 0;

            uint32_t emitCount = (std::min)(emitter_.count, (uint32_t)maxEmitCount);
            if (emitCount > 0) {
                Emitter actualEmitter = emitter_;
                actualEmitter.count = emitCount;
                particles_.splice(particles_.end(), Emit(actualEmitter, randomEngine_));
            }
            emitter_.frequencyTime -= emitter_.frequency;
        }
    }
}

void ParticleObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw) return;
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootSignature(engine_->GetPSOManager()->GetRootSignature("Particle"));
    engine_->GetDxCommon()->GetCommandList()->SetPipelineState(engine_->GetPSOManager()->GetPSO("Particle"));
    engine_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vbv_);

    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(0, instanceSrvHandleGPU_);
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(2, dirLight_->GetGPUVirtualAddress());
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(3, texture_->GetGpuHandle(textureKey));

    if (draw && numInstance_ > 0) {
        UINT vertexCount = (UINT)vertices_.size();
        engine_->GetDxCommon()->GetCommandList()->DrawInstanced(vertexCount, numInstance_, 0, 0);
    }
}

void ParticleObject::ImGuiControl(const std::string& name) {
    ImGuiSRTControl(name);
    ImGuiParticleControl(name);
    ImGui::Separator();
}

void ParticleObject::ImGuiSRTControl(const std::string& name) {
    std::string label = "##" + name;
    if (ImGui::CollapsingHeader(("SRT" + label).c_str())) {
        ImGui::DragFloat3(("scale" + label).c_str(), &emitter_.transform.scale.x, 0.01f);
        ImGui::DragFloat3(("rotate" + label).c_str(), &emitter_.transform.rotate.x, 0.01f);
        ImGui::DragFloat3(("Translate" + label).c_str(), &emitter_.transform.translate.x, 0.01f);
    }
    if (ImGui::CollapsingHeader(("Color" + label).c_str())) {
        ImGui::ColorEdit4(("Color" + label).c_str(), &materialData_->color.x, true);
    }
}

void ParticleObject::ImGuiParticleControl(const std::string& name) {
    std::string label = "##" + name;
    if (ImGui::CollapsingHeader(("Particles" + label).c_str())) {
        ImGui::Text("numInstance:%d / maxInstance:%d", numInstance_, numMaxInstance_);

        float min_dist = distribution_.a();
        float max_dist = distribution_.b();
        float min_time = distTime_.a();
        float max_time = distTime_.b();

        ImGui::DragFloat(("dist.min" + label).c_str(), &min_dist, 0.1f, -100.0f, (max_dist - 0.01f));
        ImGui::DragFloat(("dist.max" + label).c_str(), &max_dist, 0.1f, (min_dist + 0.01f), 100.0f);
        ImGui::DragFloat(("time.min" + label).c_str(), &min_time, 0.1f, 0.0f, (max_time - 0.01f));
        ImGui::DragFloat(("time.max" + label).c_str(), &max_time, 0.1f, (min_time + 0.01f), 100.0f);

        if (min_dist != distribution_.a() || max_dist != distribution_.b()) {
            distribution_ = std::uniform_real_distribution<float>(min_dist, max_dist);
        }
        if (min_time != distTime_.a() || max_time != distTime_.b()) {
            distTime_ = std::uniform_real_distribution<float>(min_time, max_time);
        }

        int maxInst = static_cast<int>(numMaxInstance_);
        if (ImGui::DragInt(("maxInstance" + label).c_str(), &maxInst, 1, 1, kNumMaxInstance)) {
            SetMaxInstance(static_cast<uint32_t>(maxInst));
        }

        int count = static_cast<int>(emitter_.count);
        if (ImGui::DragInt(("count" + label).c_str(), &count, 1, 1, numMaxInstance_)) {
            emitter_.count = static_cast<uint32_t>(count);
        }
        ImGui::DragFloat(("frequency" + label).c_str(), &emitter_.frequency, 0.1f, 0.01f, 50.0f);

        if (ImGui::Button(("+1Particle" + label).c_str())) {
            if (numInstance_ < numMaxInstance_) {
                particles_.push_back(MakeNewParticle(randomEngine_, emitter_.transform.translate));
            }
        }

        ImGui::Checkbox(("billboard" + label).c_str(), &billboardActive_);
        ImGui::Checkbox(("wind" + label).c_str(), &windActive_);
        if (windActive_) {
            ImGui::DragFloat3(("field min" + label).c_str(), &accelerationFeild_.area.min.x, 0.01f);
            ImGui::DragFloat3(("field max" + label).c_str(), &accelerationFeild_.area.max.x, 0.01f);
            ImGui::DragFloat3(("acceleration" + label).c_str(), &accelerationFeild_.acceleration.x, 0.01f);
        }
        ImGui::Separator();
        if (ImGui::Checkbox(("loop" + label).c_str(), &loopActive_)) { emitterActive_ = false; }
        if (ImGui::Checkbox(("emit" + label).c_str(), &emitterActive_)) { loopActive_ = false; }
    }
}

Particle ParticleObject::MakeNewParticle(std::mt19937& randomEngine, Vector3 startPosition) {
    Particle particle{};
    particle.transform.translate = startPosition;
    particle.transform.scale = { 1.0f, 1.0f, 1.0f };
    particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
    particle.velocity = { distribution_(randomEngine), distribution_(randomEngine), distribution_(randomEngine) };
    particle.color = { distColor_(randomEngine), distColor_(randomEngine), distColor_(randomEngine), 1.0f };
    particle.lifeTime = distTime_(randomEngine);
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
