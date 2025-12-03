#include "ParticleManager.h"
#include "Function.h"
#include "Matrix.h" 
#include "DxCommon.h"
#include "Camera.h"
#include "Collision.h"
#include <stdexcept>
#include <array>
#include <cassert>
#include "imgui.h" 

namespace {
	void InitializeTransform(Transform& transform) {
		transform.scale = { 1, 1, 1 };
		transform.rotate = { 0, 0, 0 };
		transform.translate = { 0, 0, 0 };
	}
} // namespace


ParticleManager::ParticleManager(DxCommon* dxCommon,
	const Vector3& initialPosition, const int maxInstance, const int count, const float frequency) {
	ID3D12Device* device = dxCommon->GetDevice();
	ID3D12DescriptorHeap* srvHeap = dxCommon->GetSrvHeap();

	// Transform初期化
	InitializeTransform(emitter_.transform);
	emitter_.transform.translate = initialPosition;
	emitter_.count = count;
	emitter_.frequency = frequency;
	emitter_.frequencyTime = 0.0f;

	// パーティクルの数の初期化
	numMaxInstance_ = maxInstance;
	if (numMaxInstance_ > kNumMaxInstance) { numMaxInstance_ = kNumMaxInstance; };

	// 風の初期化
	accelerationFeild_.acceleration = { 50.0f,0.0f,0.0f };
	accelerationFeild_.area.min = { -10.0f,-10.0f,10.0f };
	accelerationFeild_.area.max = { 10.0f,10.0f,30.0f };

	// Materialリソース作成
	materialResource_ = CreateBufferResource(device, sizeof(Material));
	if (!materialResource_) throw std::runtime_error("Failed to create materialResource_");
	HRESULT hr_mat = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	if (FAILED(hr_mat) || !materialData_) throw std::runtime_error("Failed to map materialResource_");
	materialData_->color = { 1,1,1,1 };
	materialData_->enableLighting = 0;
	materialData_->uvtransform = Math::MakeIdentity();

	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ------------------------------------
	// 1. 板ポリゴン（クアッド）の頂点データ作成
	// ------------------------------------
	vertices_ = {
		// Triangle 1
		 {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		 {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		 {{1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},

		 // Triangle 2
		 {{1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		 {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		 {{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}
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


	// ------------------------------------
	// 2. インスタンスデータ用 StructuredBuffer 作成
	// ------------------------------------
	size_t bufferSize = sizeof(ParticleForGPU) * numMaxInstance_;
	instanceResource_ = CreateBufferResource(device, bufferSize);
	if (!instanceResource_) throw std::runtime_error("Failed to create instanceResource_");

	HRESULT hr = instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
	if (FAILED(hr) || !instanceData_) throw std::runtime_error("Failed to map instanceResource_");

	// ------------------------------------
	// 3. インスタンスごとのTransformを初期化
	// ------------------------------------
	randomEngine_ = std::mt19937(seedGenerator_());

	// ------------------------------------
	// 4. インスタンスデータ用 SRVの作成
	// ------------------------------------

	instanceSrvHandleCPU_ =
		GetCPUDescriptorHandle(srvHeap, descriptorSize, kDescriptorIndex);

	instanceSrvHandleGPU_ =
		GetGPUDescriptorHandle(srvHeap, descriptorSize, kDescriptorIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = numMaxInstance_;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	device->CreateShaderResourceView(
		instanceResource_.Get(),
		&instancingSrvDesc,
		instanceSrvHandleCPU_
	);
	kDescriptorIndex++;
}

void ParticleManager::Update(const Camera* camera) {
	numInstance_ = 0;

	Matrix4x4 managerWorldMatrix = Math::MakeAffineMatrix(
		emitter_.transform.scale,
		emitter_.transform.rotate,
		emitter_.transform.translate
	);

	Matrix4x4 billBoardMatrix = Math::MakeIdentity();
	if (billboardActive_) {
		billBoardMatrix = Math::Inverse(camera->GetViewMatrix3D());
		billBoardMatrix.m[3][0] = 0.0f;
		billBoardMatrix.m[3][1] = 0.0f;
		billBoardMatrix.m[3][2] = 0.0f;
	}

	for (auto particleIterator = particles_.begin();
		particleIterator != particles_.end();) {

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

		Matrix4x4 worldViewProjection = Math::Multiply(particleWorldMatrix, Math::Multiply(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D()));
		Matrix4x4 worldMatrix = particleWorldMatrix;

		// Feildの範囲内のParticleには加速度を適応する
		if (IsCollision(accelerationFeild_.area, (*particleIterator).transform.translate)) {
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
		// [ループモード]:
		size_t currentParticleCount = particles_.size();
		size_t neededCount = numMaxInstance_ - currentParticleCount;

		for (size_t i = 0; i < neededCount; ++i) {
			particles_.push_back(MakeNewParticle(randomEngine_, emitter_.transform.translate));
		}
	}

	if (emitterActive_ && !loopActive_) {
		// [エミッタモード]:
		emitter_.frequencyTime += kDeltaTime_;
		if (emitter_.frequency <= emitter_.frequencyTime) {

			size_t currentParticleCount = particles_.size();
			size_t maxEmitCount = numMaxInstance_ - currentParticleCount;

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


void ParticleManager::Draw(ID3D12GraphicsCommandList* commandList,
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
	D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
	ID3D12PipelineState* pipelineState,
	ID3D12RootSignature* rootSignature,
	bool draw) {

	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->SetPipelineState(pipelineState);

	commandList->IASetVertexBuffers(0, 1, &vbv_);

	// Root Parameter 0: インスタンスデータ
	commandList->SetGraphicsRootDescriptorTable(0, instanceSrvHandleGPU_);

	// Root Parameter 1: マテリアル
	commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());

	// Root Parameter 2: DirectionalLight
	commandList->SetGraphicsRootConstantBufferView(2, lightAddress);

	// Root Parameter 3: テクスチャ
	commandList->SetGraphicsRootDescriptorTable(3, textureHandle);

	if (draw) {
		UINT vertexCount = (UINT)vertices_.size();
		assert(vertexCount > 0 && "Vertex count must be greater than 0");
		if (numInstance_ > 0) {
			commandList->DrawInstanced(vertexCount, numInstance_, 0, 0);
		}
	}
}

void ParticleManager::ImGuiControl(const std::string& name) {
	ImGuiSRTControl(name);
	ImGuiParticleControl(name);
	ImGui::Separator();
}

void ParticleManager::ImGuiSRTControl(const std::string& name) {
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

void ParticleManager::ImGuiParticleControl(const std::string& name) {
	std::string label = "##" + name;

	if (ImGui::CollapsingHeader(("Particles" + label).c_str())) {
		ImGui::Text("numInstance:%d / maxInstance:%d", numInstance_, numMaxInstance_);
		// 現在のパラメータを取得
		float min_dist = distribution_.a(); // 下限
		float max_dist = distribution_.b(); // 上限
		float min_time = distTime_.a();     // 下限
		float max_time = distTime_.b();     // 上限

		// ImGuiでローカル変数 (min_dist, max_dist, min_time, max_time) を編集
		ImGui::DragFloat(("distribution.min" + label).c_str(), &min_dist, 0.1f, -100.0f, (max_dist - 0.01f));
		ImGui::DragFloat(("distribution.max" + label).c_str(), &max_dist, 0.1f, (min_dist + 0.01f), 100.0f);
		ImGui::DragFloat(("distTime.min" + label).c_str(), &min_time, 0.1f, 0.0f, (max_time - 0.01f));
		ImGui::DragFloat(("distTime.max" + label).c_str(), &max_time, 0.1f, (min_time + 0.01f), 100.0f);

		// 変更された値を新しいパラメータオブジェクトとして設定し直す
		if (min_dist != distribution_.a() || max_dist != distribution_.b()) {
			distribution_ = std::uniform_real_distribution<float>(min_dist, max_dist);
		}
		if (min_time != distTime_.a() || max_time != distTime_.b()) {
			distTime_ = std::uniform_real_distribution<float>(min_time, max_time);
		}

		// count
		int count = static_cast<int>(emitter_.count);
		if (ImGui::DragInt(("count" + label).c_str(), &count, 1, 1, numMaxInstance_)) {
			emitter_.count = static_cast<uint32_t>(count);
		}
		// frequency
		ImGui::DragFloat(("frequency" + label).c_str(), &emitter_.frequency, 0.1f, 0.01f, 50.0f);

		if (ImGui::Button(("+1Particle" + label).c_str())) {
			if (numInstance_ < numMaxInstance_) {
				particles_.push_back(MakeNewParticle(randomEngine_, emitter_.transform.translate));
			}
		}
		if (ImGui::Button(("+Emitter.count" + label).c_str())) {
			if (numInstance_ <= (numMaxInstance_ - emitter_.count)) {
				particles_.splice(particles_.end(), Emit(emitter_, randomEngine_));
			}
		}
		ImGui::Checkbox(("billboard" + label).c_str(), &billboardActive_);
		ImGui::Separator();
		if (ImGui::Checkbox(("loop" + label).c_str(), &loopActive_)) { emitterActive_ = false; }
		if (ImGui::Checkbox(("emit" + label).c_str(), &emitterActive_)) { loopActive_ = false; }
	}
}

Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, Vector3 startPosition) {
	Particle particle{};
	particle.transform.translate = startPosition;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.velocity = { distribution_(randomEngine), distribution_(randomEngine), distribution_(randomEngine) };
	particle.color = { distColor_(randomEngine),distColor_(randomEngine), distColor_(randomEngine), 1.0f };
	particle.lifeTime = distTime_(randomEngine);
	particle.currentTime = 0.0f;
	return particle;
}

std::list<Particle> ParticleManager::Emit(const Emitter& emitter, std::mt19937& randomEngine) {
	std::list<Particle> particles;
	for (uint32_t count = 0; count < emitter.count; ++count) {
		particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
	}
	return particles;
}
