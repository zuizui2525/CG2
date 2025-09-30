#include "SpriteObject.h"
#include "Function.h"

SpriteObject::SpriteObject(ID3D12Device* device, float width, float height, const Vector3& position)
    : Object3D(device, 0) // 0 = ライティング無効
{
    // 頂点リソース作成 (4頂点)
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = sizeof(VertexData) * 4;
    vbv_.StrideInBytes = sizeof(VertexData);

    VertexData* vData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
    vData[0].position = { 0.0f, height, 0.0f, 1.0f }; vData[0].texcoord = { 0,1 }; vData[0].normal = { 0,0,-1 };
    vData[1].position = { 0.0f, 0.0f,   0.0f, 1.0f }; vData[1].texcoord = { 0,0 }; vData[1].normal = { 0,0,-1 };
    vData[2].position = { width,height, 0.0f, 1.0f }; vData[2].texcoord = { 1,1 }; vData[2].normal = { 0,0,-1 };
    vData[3].position = { width,0.0f,   0.0f, 1.0f }; vData[3].texcoord = { 1,0 }; vData[3].normal = { 0,0,-1 };

    // インデックスリソース作成 (2三角形)
    indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * 6);
    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = sizeof(uint32_t) * 6;
    ibv_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* iData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
    iData[0] = 0; iData[1] = 1; iData[2] = 2; iData[3] = 1; iData[4] = 3; iData[5] = 2;

    // Object3D の materialResource_ を Map して保持
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    // 位置を初期化
    transform_.translate = position;
}

void SpriteObject::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );
    Matrix4x4 worldViewProjection = Math::Multiply(Math::Multiply(worldMatrix, viewMatrix), projectionMatrix);

    TransformationMatrix* wvpData = nullptr;
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    wvpData->WVP = worldViewProjection;
    wvpData->world = worldMatrix;
}

void SpriteObject::UpdateUVTransform() {
    // Scale → Rotate → Translate の順で合成
    Matrix4x4 uvMatrix = Math::MakeScaleMatrix(uvTransform_.scale);
    uvMatrix = Math::Multiply(uvMatrix, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uvMatrix = Math::Multiply(uvMatrix, Math::MakeTranslateMatrix(uvTransform_.translate));

    // マテリアルに反映
    materialData_->uvtransform = uvMatrix;
}

void SpriteObject::Draw(ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* directionalLightResource,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    bool draw) {
    if (!draw) return;

    commandList->IASetVertexBuffers(0, 1, &vbv_);
    commandList->IASetIndexBuffer(&ibv_);

    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    commandList->SetGraphicsRootDescriptorTable(2, textureHandle);

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
