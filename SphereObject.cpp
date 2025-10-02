#include "SphereObject.h"

SphereObject::SphereObject(ID3D12Device* device, uint32_t subdivision, float radius)
    : subdivision_(subdivision), radius_(radius) {
    // 頂点数とインデックス数を計算
    uint32_t kVertexCount = (subdivision_ + 1) * (subdivision_ + 1);
    uint32_t kIndexCount = subdivision_ * subdivision_ * 6;
    float kLonEvery = static_cast<float>(M_PI * 2.0f / subdivision_);
    float kLatEvery = static_cast<float>(M_PI / subdivision_);

    // Material
    materialResource_ = CreateBufferResource(device, sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 2;
    materialData_->uvtransform = Math::MakeIdentity();

    // WVP
    wvpResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Vertex
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * kVertexCount);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* vtx;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));

    for (uint32_t latIndex = 0; latIndex <= subdivision_; ++latIndex) {
        float lat = static_cast<float>(-M_PI / 2.0f + kLatEvery * latIndex);
        for (uint32_t lonIndex = 0; lonIndex <= subdivision_; ++lonIndex) {
            float lon = kLonEvery * lonIndex;

            float x = cosf(lat) * cosf(lon) * radius_;
            float y = sinf(lat) * radius_;
            float z = cosf(lat) * sinf(lon) * radius_;

            uint32_t index = latIndex * (subdivision_ + 1) + lonIndex;
            vtx[index].position = { x, y, z, 1.0f };
            vtx[index].texcoord = { (float)lonIndex / subdivision_, 1.0f - (float)latIndex / subdivision_ };
            vtx[index].normal = { x, y, z }; // 法線ベクトル
        }
    }

    // Index
    indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * kIndexCount);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
    ibView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* idxGPU;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idxGPU));

    uint32_t idx = 0;
    for (uint32_t latIndex = 0; latIndex < subdivision_; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < subdivision_; ++lonIndex) {
            uint32_t current = latIndex * (subdivision_ + 1) + lonIndex;
            uint32_t next = current + subdivision_ + 1;

            // 三角形1
            idxGPU[idx++] = current;
            idxGPU[idx++] = next;
            idxGPU[idx++] = current + 1;

            // 三角形2
            idxGPU[idx++] = current + 1;
            idxGPU[idx++] = next;
            idxGPU[idx++] = next + 1;
        }
    }
}

SphereObject::~SphereObject() {
    // ComPtrなので明示解放は不要
}

void SphereObject::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, viewMatrix), projectionMatrix);
    wvpData_->WVP = wvp;
    wvpData_->world = world;

    Matrix4x4 uv = Math::MakeScaleMatrix(uvTransform_.scale);
    uv = Math::Multiply(uv, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uv = Math::Multiply(uv, Math::MakeTranslateMatrix(uvTransform_.translate));
    materialData_->uvtransform = uv;
}

void SphereObject::Draw(ID3D12GraphicsCommandList* commandList,
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

    // インデックス数 = subdivision * subdivision * 6
    uint32_t indexCount = subdivision_ * subdivision_ * 6;
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}
