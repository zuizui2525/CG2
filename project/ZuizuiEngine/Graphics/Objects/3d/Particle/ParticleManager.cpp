#include "ParticleManager.h"
#include "Function.h" // GetCPUDescriptorHandle, GetGPUDescriptorHandle, CreateBufferResource を利用
#include "Matrix.h" 
#include "DxCommon.h" // GetDevice(), GetSrvHeap() を利用
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

    // CBV/SRV/UAV ヒープのディスクリプタサイズを取得
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // ------------------------------------
    // 1. 板ポリゴン（クアッド）の頂点データ作成
    // ------------------------------------
    // クアッドの頂点データ (6頂点)
    vertices_ = {
        //   Pos.x   Pos.y   Pos.z   Pos.w   U.u    U.v    Norm.x Norm.y Norm.z
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
    size_t bufferSize = sizeof(TransformationMatrix) * kNumInstance;
    instanceResource_ = CreateBufferResource(device, bufferSize);
    if (!instanceResource_) throw std::runtime_error("Failed to create instanceResource_");

    // マッピング
    HRESULT hr = instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
    if (FAILED(hr) || !instanceData_) throw std::runtime_error("Failed to map instanceResource_");

    // ------------------------------------
    // 3. インスタンスごとのTransformを初期化 
    // ------------------------------------
    for (UINT index = 0; index < kNumInstance; ++index) {
        transforms_[index].translate = initialPosition;
        float offset = (float)index * 0.1f;
        transforms_[index].translate.x += offset;
        transforms_[index].translate.y += offset;
        transforms_[index].scale = { 1.0f, 1.0f, 1.0f };
        transforms_[index].rotate = { 0.0f, 0.0f, 0.0f };
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
    instancingSrvDesc.Buffer.NumElements = kNumInstance;
    instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);

    // SRVを作成
    device->CreateShaderResourceView(
        instanceResource_.Get(),
        &instancingSrvDesc,
        instanceSrvHandleCPU_ // 保持したCPUハンドルを使用
    );
}

// ------------------------------------
// Update
// ------------------------------------
void ParticleManager::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    // 全てのインスタンスのWVP行列を計算し、GPUバッファに書き込む
    for (UINT index = 0; index < kNumInstance; ++index) {
        Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
            transforms_[index].scale,
            transforms_[index].rotate,
            transforms_[index].translate
        );

        Matrix4x4 worldViewProjection = Math::Multiply(Math::Multiply(worldMatrix, viewMatrix), projectionMatrix);

        // インスタンスデータに書き込み
        instanceData_[index].WVP = worldViewProjection;
        instanceData_[index].world = worldMatrix;
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
        // 1つのメッシュ(クアッド)を kNumInstance 回描画する
        commandList->DrawInstanced(vertexCount, kNumInstance, 0, 0);
    }
}
