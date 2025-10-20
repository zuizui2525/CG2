#include "SpriteObject.h"

SpriteObject::SpriteObject(ID3D12Device* device, int width, int height) {
    // Material
    materialResource_ = CreateBufferResource(device, sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 0;
    materialData_->uvtransform = Math::MakeIdentity();

    // WVP
    wvpResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Vertex (四角形)
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 4;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* vtx;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
    vtx[0] = { {0.0f, (float)height, 0.0f, 1.0f}, {0,1}, {0,0,-1} };
    vtx[1] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0,0}, {0,0,-1} };
    vtx[2] = { {(float)width, (float)height, 0.0f, 1.0f}, {1,1}, {0,0,-1} };
    vtx[3] = { {(float)width, 0.0f, 0.0f, 1.0f}, {1,0}, {0,0,-1} };

    // Index
    indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * 6);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * 6;
    ibView_.Format = DXGI_FORMAT_R32_UINT;
    uint32_t* idx;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idx));
    idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 1; idx[4] = 3; idx[5] = 2;
}

SpriteObject::~SpriteObject() {}

void SpriteObject::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, viewMatrix), projectionMatrix);
    wvpData_->WVP = wvp;
    wvpData_->world = world;

    Matrix4x4 uv = Math::MakeScaleMatrix(uvTransform_.scale);
    uv = Math::Multiply(uv, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uv = Math::Multiply(uv, Math::MakeTranslateMatrix(uvTransform_.translate));
    materialData_->uvtransform = uv;
}

void SpriteObject::Draw(ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    ID3D12Resource* directionalLightResource,
    bool enableDraw) {
    if (!enableDraw) return;

    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->IASetIndexBuffer(&ibView_);
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, textureHandle);

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
