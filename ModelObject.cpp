#include "ModelObject.h"
#include "Function.h"

ModelObject::ModelObject(ID3D12Device* device,
    const std::string& directory,
    const std::string& filename)
    : Object3D(device, 2) // 2 = モデル用 Lighting 設定
{
    // objファイル読み込み
    modelData_ = LoadObjFile(directory, filename);

    // 頂点リソース作成
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * modelData_.vertices.size());

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vbv_.StrideInBytes = sizeof(VertexData);

    VertexData* vData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
    memcpy(vData, modelData_.vertices.data(),
        sizeof(VertexData) * modelData_.vertices.size());

    // Object3D で作成済みの materialResource_ を Map してポインタを保持
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
}

void ModelObject::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    // ワールド行列
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );

    // WVP行列
    Matrix4x4 worldViewProjection = Math::Multiply(Math::Multiply(worldMatrix, viewMatrix), projectionMatrix);

    // 定数バッファに書き込み
    TransformationMatrix* wvpData = nullptr;
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    wvpData->WVP = worldViewProjection;
    wvpData->world = worldMatrix;
}

void ModelObject::Draw(ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* materialResource,
    ID3D12Resource* wvpResource,
    ID3D12Resource* directionalLightResource,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    bool draw) {
    // VBV設定
    commandList->IASetVertexBuffers(0, 1, &vbv_);

    // 定数バッファ設定
    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    // テクスチャ
    commandList->SetGraphicsRootDescriptorTable(2, textureHandle);

    // 描画
    if (draw) {
        commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
    }
}