#include "TriangleObject.h"
#include "Function.h"
#include "Camera.h"

void TriangleObject::Initialize(ID3D12Device* device, int lightingMode) {
    // 基底クラスの初期化
    Object3D::Initialize(device, lightingMode);

    // Vertex (三角形3頂点)
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 3);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 3;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* vtx;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
    vtx[0].position = { -0.5f, -0.5f, 0.0f, 1.0f }; // 左下
    vtx[0].texcoord = { 0.0f, 1.0f };
    vtx[0].normal = { 0.0f, 0.0f, -1.0f };

    vtx[1].position = { 0.0f, 0.5f, 0.0f, 1.0f }; // 上
    vtx[1].texcoord = { 0.5f, 0.0f };
    vtx[1].normal = { 0.0f, 0.0f, -1.0f };

    vtx[2].position = { 0.5f, -0.5f, 0.0f, 1.0f }; // 右下
    vtx[2].texcoord = { 1.0f, 1.0f };
    vtx[2].normal = { 0.0f, 0.0f, -1.0f };
}

void TriangleObject::Update(const Camera* camera) {
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

void TriangleObject::Draw(ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
    D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
    ID3D12PipelineState* pipelineState,
    ID3D12RootSignature* rootSignature,
    bool enableDraw) {
    if (!enableDraw) return;

    // パイプラインの選択
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);

    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, lightAddress);
    commandList->SetGraphicsRootConstantBufferView(3, cameraAddress);
    commandList->SetGraphicsRootDescriptorTable(4, textureHandle);

    commandList->DrawInstanced(3, 1, 0, 0);
}
