#include "ParticleManager.h"
#include "Function.h"
#include "Matrix.h" 
#include "DxCommon.h"
#include "Camera.h"
#include <stdexcept>
#include <array>
#include <cassert>

ParticleManager::ParticleManager(DxCommon* dxCommon,
	const Vector3& initialPosition)
	// Object3Dの初期化: ライティングモード0 (無効) を仮定
	: Object3D(dxCommon->GetDevice(), 0) {
	// DxCommonから必要な情報を取得
	ID3D12Device* device = dxCommon->GetDevice();
	ID3D12DescriptorHeap* srvHeap = dxCommon->GetSrvHeap();
	transform_.translate = initialPosition;

	// CBV/SRV/UAV ヒープのディスクリプタサイズを取得
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ------------------------------------
	// 1. 板ポリゴン（クアッド）の頂点データ作成
	// ------------------------------------
	// クアッドの頂点データ (6頂点)
	vertices_ = {
		// Triangle 1
		 {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
		 {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 左上
		 {{1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},  // 右下

		 // Triangle 2
		 {{1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},  // 右下
		 {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 左上
		 {{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}   // 右上
	};

	// 頂点リソース作成
	size_t vertexBufferSize = sizeof(VertexData) * vertices_.size();
	vertexResource_ = CreateBufferResource(device, vertexBufferSize);
	if (!vertexResource_) throw std::runtime_error("Failed to create vertexResource_");

	// 頂点データ転送
	VertexData* vertexData = nullptr;
	HRESULT map_hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	if (FAILED(map_hr)) throw std::runtime_error("Failed to map vertexResource_");
	memcpy(vertexData, vertices_.data(), vertexBufferSize);

	// VBV (Vertex Buffer View) の設定
	vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vbv_.SizeInBytes = (UINT)vertexBufferSize;
	vbv_.StrideInBytes = sizeof(VertexData);

	// ------------------------------------
	// 2. インスタンスデータ用 StructuredBuffer 作成
	// ------------------------------------
	size_t bufferSize = sizeof(ParticleForGPU) * kNumMaxInstance;
	instanceResource_ = CreateBufferResource(device, bufferSize);
	if (!instanceResource_) throw std::runtime_error("Failed to create instanceResource_");

	// マッピング
	HRESULT hr = instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
	if (FAILED(hr) || !instanceData_) throw std::runtime_error("Failed to map instanceResource_");

	// ------------------------------------
	// 3. インスタンスごとのTransformを初期化 
	// ------------------------------------
	// ランダム
	randomEngine_ = std::mt19937(seedGenerator_());

	for (UINT index = 0; index < kNumMaxInstance; ++index) {
		particles_[index] = MakeNewParticle(randomEngine_, transform_.translate);
	}

	// ------------------------------------
	// 4. インスタンスデータ用 SRVの作成 (Function.hのヘルパー関数を使用)
	// ------------------------------------

	// ★ CPUハンドルを計算し、メンバ変数に保持
	instanceSrvHandleCPU_ =
		GetCPUDescriptorHandle(srvHeap, descriptorSize, kDescriptorIndex);

	// ★ GPUハンドルを計算し、メンバ変数に保持
	instanceSrvHandleGPU_ =
		GetGPUDescriptorHandle(srvHeap, descriptorSize, kDescriptorIndex);

	// SRVディスクリプタの設定 (Structured Bufferとして設定)
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN; // Structured BufferはUNKNOWN
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	// SRVを作成
	device->CreateShaderResourceView(
		instanceResource_.Get(),
		&instancingSrvDesc,
		instanceSrvHandleCPU_ // 保持したCPUハンドルを使用
	);
	kDescriptorIndex++;
}

// ------------------------------------
// Update
// ------------------------------------
void ParticleManager::Update(const Camera* camera) {

	// 【★追加】ParticleManager自身のワールド行列を計算
	// Object3Dから継承した transform_ メンバを使用
	Matrix4x4 managerWorldMatrix = Math::MakeAffineMatrix(
		transform_.scale,
		transform_.rotate,
		transform_.translate
	);

	// numInstance_のリセット
	numInstance_ = 0;
	// 全てのインスタンスのWVP行列を計算し、GPUバッファに書き込む
	for (UINT index = 0; index < kNumMaxInstance; ++index) {
		if (particles_[index].lifeTime <= particles_[index].currentTime) {
			particles_[index] = MakeNewParticle(randomEngine_, transform_.translate);
			continue;
		}

		// 【修正】パーティクル個別のローカルワールド行列を計算
		Matrix4x4 particleLocalWorldMatrix = Math::MakeAffineMatrix(
			particles_[index].transform.scale,
			particles_[index].transform.rotate,
			particles_[index].transform.translate
		);

		Matrix4x4 billboardMatrix = camera->GetCameraMatrix();
		billboardMatrix.m[3][0] = 0.0f;
		billboardMatrix.m[3][1] = 0.0f;
		billboardMatrix.m[3][2] = 0.0f;
		billboardMatrix.m[3][3] = 1.0f;

		// ビルボードするかしないか
		Matrix4x4 worldMatrix{};
		if (billboardActive_) {
			worldMatrix = Math::Multiply(Math::Multiply(particleLocalWorldMatrix, managerWorldMatrix), billboardMatrix);
		} else {
			worldMatrix = Math::Multiply(particleLocalWorldMatrix, managerWorldMatrix);
		}

		Matrix4x4 worldViewProjection = Math::Multiply(Math::Multiply(worldMatrix, camera->GetViewMatrix3D()), camera->GetProjectionMatrix3D());

		// インスタンスデータに書き込み
		particles_[index].transform.translate += particles_[index].velocity * kDeltaTime_;
		particles_[index].currentTime += kDeltaTime_;
		alpha_ = 1.0f - (particles_[index].currentTime / particles_[index].lifeTime);
		instanceData_[numInstance_].WVP = worldViewProjection;
		instanceData_[numInstance_].world = worldMatrix;
		instanceData_[numInstance_].color = particles_[index].color;
		instanceData_[numInstance_].color.w = alpha_;
		++numInstance_;
	}
}


// ------------------------------------
// Draw (RootSignatureの定義に完全に対応)
// ------------------------------------
void ParticleManager::Draw(ID3D12GraphicsCommandList* commandList,
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
	D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
	ID3D12PipelineState* pipelineState,
	ID3D12RootSignature* rootSignature,
	bool draw) {

	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->SetPipelineState(pipelineState);

	// VBV設定（板ポリの頂点データ）
	commandList->IASetVertexBuffers(0, 1, &vbv_);

	// 【Root Parameter 0: インスタンスデータ (SRV, t0, VS)】
	// インスタンシング用の行列データ。内部で保持しているGPUハンドルを使用
	commandList->SetGraphicsRootDescriptorTable(0, instanceSrvHandleGPU_);

	// 【Root Parameter 1: マテリアル (CBV, b0, PS)】
	commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());

	// 【Root Parameter 2: DirectionalLight (CBV, b1, PS)】
	commandList->SetGraphicsRootConstantBufferView(2, lightAddress);

	// 【Root Parameter 3: テクスチャ (SRV, t0, PS)】
	// 外部から渡されたテクスチャのGPUハンドルを使用
	commandList->SetGraphicsRootDescriptorTable(3, textureHandle);

	// 描画: インスタンシング描画
	if (draw) {
		UINT vertexCount = (UINT)vertices_.size();
		assert(vertexCount > 0 && "Vertex count must be greater than 0");
		// 1つのメッシュ(クアッド)を numInstance 回描画する
		commandList->DrawInstanced(vertexCount, numInstance_, 0, 0);
	}
}

void ParticleManager::ImGuiParticleControl(const std::string& name) {
	std::string label = "##" + name;
	
	if (ImGui::Button(("Reset" + label).c_str())) {
		for (UINT index = 0; index < kNumMaxInstance; ++index) {
			particles_[index] = MakeNewParticle(randomEngine_, transform_.translate);
		}
    }  
	ImGui::Checkbox(("billboard" + label).c_str(), &billboardActive_);
	ImGui::Separator();
	ImGui::Text("numInstance:%d / maxInstance:%d", numInstance_, kNumMaxInstance);
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
