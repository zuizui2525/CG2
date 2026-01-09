#include "ModelObject.h"
#include "ModelManager.h"
#include "Function.h"
#include "Camera.h"

ModelObject::ModelObject(ID3D12Device* device,
    const std::string& filename,
    const Vector3& initialPosition)
    : Object3D(device, 2) // 2 = ライティング有効
{
    // 初期位置をセット
    transform_.translate = initialPosition;

    // モデルデータ読み込み
    modelData_ = ModelManager::GetInstance().LoadModel(device, filename);

    // 頂点リソース作成
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * modelData_->vertices.size());
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = sizeof(VertexData) * (UINT)modelData_->vertices.size();
    vbv_.StrideInBytes = sizeof(VertexData);

    // 頂点データ転送
    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    memcpy(vertexData, modelData_->vertices.data(), sizeof(VertexData) * modelData_->vertices.size());
}

void ModelObject::Update(const Camera* camera) {
    // ワールド行列
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );

    // WVP行列
    Matrix4x4 worldViewProjection = Math::Multiply(Math::Multiply(worldMatrix, camera->GetViewMatrix3D()), camera->GetProjectionMatrix3D());

    // 逆転置行列
    Matrix4x4 worldForNormal = worldViewProjection;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    // 定数バッファに書き込み
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = worldViewProjection;
    wvpData_->world = worldMatrix;
    wvpData_->WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
}

void ModelObject::Draw(ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
    D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
    ID3D12PipelineState* pipelineState,
    ID3D12RootSignature* rootSignature,
    bool draw) {
    
    // パイプラインの選択
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);
    
    // VBV設定
    commandList->IASetVertexBuffers(0, 1, &vbv_);

    // 定数バッファ設定
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, lightAddress);
    commandList->SetGraphicsRootConstantBufferView(3, cameraAddress);
    commandList->SetGraphicsRootDescriptorTable(4, textureHandle);

    // 描画
    if (draw) {
        commandList->DrawInstanced(UINT(modelData_->vertices.size()), 1, 0, 0);
    }
}
