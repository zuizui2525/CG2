#include "SphereObject.h"
#include "Function.h"
#include "Camera.h"
#include "Matrix.h"

SphereObject::SphereObject(ID3D12Device* device)
    : Object3D(device, 2) {
    device_ = device;
    // 初回のメッシュ生成
    CreateMesh();
}

SphereObject::~SphereObject() {
}

void SphereObject::CreateMesh() {
    // 頂点数とインデックス数を計算
    uint32_t kVertexCount = (subdivision_ + 1) * (subdivision_ + 1);
    uint32_t kIndexCount = subdivision_ * subdivision_ * 6;
    float kLonEvery = static_cast<float>(M_PI * 2.0f / subdivision_);
    float kLatEvery = static_cast<float>(M_PI / subdivision_);

    // Vertex Resource 作成 (以前のリソースはComPtrの代入により自動解放される)
    vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * kVertexCount);
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
            vtx[index].normal = { x, y, z };
        }
    }
    vertexResource_->Unmap(0, nullptr);

    // Index Resource 作成
    indexResource_ = CreateBufferResource(device_, sizeof(uint32_t) * kIndexCount);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
    ibView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* idxGPU = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idxGPU));

    uint32_t idx = 0;
    for (uint32_t latIndex = 0; latIndex < subdivision_; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < subdivision_; ++lonIndex) {
            uint32_t current = latIndex * (subdivision_ + 1) + lonIndex;
            uint32_t next = current + subdivision_ + 1;

            idxGPU[idx++] = current;
            idxGPU[idx++] = next;
            idxGPU[idx++] = current + 1;

            idxGPU[idx++] = current + 1;
            idxGPU[idx++] = next;
            idxGPU[idx++] = next + 1;
        }
    }
    indexResource_->Unmap(0, nullptr);
}

void SphereObject::Update(const Camera* camera) {
    // パラメータに変更があればメッシュを再生成
    if (needsUpdate_) {
        CreateMesh();
        needsUpdate_ = false;
    }

    // 行列更新
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, camera->GetViewMatrix3D()), camera->GetProjectionMatrix3D());

    Matrix4x4 worldForNormal = world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    wvpData_->WVP = wvp;
    wvpData_->world = world;
    wvpData_->WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    Matrix4x4 uv = Math::MakeScaleMatrix(uvTransform_.scale);
    uv = Math::Multiply(uv, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uv = Math::Multiply(uv, Math::MakeTranslateMatrix(uvTransform_.translate));
    materialData_->uvtransform = uv;
}

void SphereObject::Draw(ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
    D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
    ID3D12PipelineState* pipelineState,
    ID3D12RootSignature* rootSignature,
    bool enableDraw) {
    if (!enableDraw) return;

    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);

    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->IASetIndexBuffer(&ibView_);
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, lightAddress);
    commandList->SetGraphicsRootConstantBufferView(3, cameraAddress);
    commandList->SetGraphicsRootDescriptorTable(4, textureHandle);

    uint32_t indexCount = subdivision_ * subdivision_ * 6;
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}
