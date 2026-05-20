#include "MeshParticleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include <numbers>
#include <cmath>

void MeshParticleObject::Initialize(int lightingMode) {
    BaseParticleObject::Initialize(lightingMode);
    if (setting_.meshType == "flat_ring") {
        CreateFlatRingMesh();
    } else if (setting_.meshType == "cylinder") {
        CreateCylinderMesh();
    } else {
        CreateCubeMesh();
    }
}

void MeshParticleObject::CreateCubeMesh() {
    ID3D12Device* device = sEngine->GetDevice();

    // 立方体の頂点データ
    std::vector<VertexData> vertices = {
        // 前面
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f,  1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f,  1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        // 背面
        {{ 1.0f, -1.0f,  1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}},
        {{-1.0f, -1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f,  1.0f}},
        {{-1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f,  1.0f}},
        // 左面
        {{-1.0f, -1.0f,  1.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        // 右面
        {{ 1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}},
        // 上面
        {{-1.0f,  1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f,  1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f,  1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f,  1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f,  1.0f, 0.0f}},
        // 下面
        {{-1.0f, -1.0f,  1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
    };

    std::vector<uint32_t> indices = {
         0, 1, 2,  2, 1, 3, // 前
         4, 5, 6,  6, 5, 7, // 後
         8, 9,10, 10, 9,11, // 左
        12,13,14, 14,13,15, // 右
        16,17,18, 18,17,19, // 上
        20,21,22, 22,21,23  // 下
    };
    indexCount_ = (uint32_t)indices.size();

    size_t vertexBufferSize = sizeof(VertexData) * vertices.size();
    vertexResource_ = DxUtils::CreateBufferResource(device, vertexBufferSize);
    VertexData* vData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
    memcpy(vData, vertices.data(), vertexBufferSize);
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = (UINT)vertexBufferSize;
    vbv_.StrideInBytes = sizeof(VertexData);

    size_t indexBufferSize = sizeof(uint32_t) * indices.size();
    indexResource_ = DxUtils::CreateBufferResource(device, indexBufferSize);
    uint32_t* iData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
    memcpy(iData, indices.data(), indexBufferSize);
    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = (UINT)indexBufferSize;
    ibv_.Format = DXGI_FORMAT_R32_UINT;
}

void MeshParticleObject::CreateFlatRingMesh() {
    ID3D12Device* device = sEngine->GetDevice();
    uint32_t kRingDivide = 32;
    float kOuterRadius = 1.0f;
    float kInnerRadius = 0.2f;

    uint32_t kVertexCount = (kRingDivide + 1) * 2;
    uint32_t kIndexCount = kRingDivide * 6;

    std::vector<VertexData> vertices(kVertexCount);
    float radianPerDivide = static_cast<float>(std::numbers::pi_v<float> * 2.0f / kRingDivide);

    uint32_t vIndex = 0;
    for (uint32_t i = 0; i <= kRingDivide; ++i) {
        float theta = i * radianPerDivide;
        float s = std::sin(theta);
        float c = std::cos(theta);
        float u = static_cast<float>(i) / kRingDivide;

        Vector3 normal = { 0.0f, 0.0f, -1.0f };

        vertices[vIndex].position = { -s * kOuterRadius, c * kOuterRadius, 0.0f, 1.0f };
        vertices[vIndex].normal = normal;
        vertices[vIndex].texcoord = { u, 0.0f };
        vIndex++;

        vertices[vIndex].position = { -s * kInnerRadius, c * kInnerRadius, 0.0f, 1.0f };
        vertices[vIndex].normal = normal;
        vertices[vIndex].texcoord = { u, 1.0f };
        vIndex++;
    }

    std::vector<uint32_t> indices(kIndexCount);
    uint32_t iIndex = 0;
    for (uint32_t i = 0; i < kRingDivide; ++i) {
        uint32_t vOuter = i * 2;
        uint32_t vInner = i * 2 + 1;
        uint32_t vNextOuter = (i + 1) * 2;
        uint32_t vNextInner = (i + 1) * 2 + 1;

        indices[iIndex++] = vOuter;
        indices[iIndex++] = vInner;
        indices[iIndex++] = vNextOuter;

        indices[iIndex++] = vNextOuter;
        indices[iIndex++] = vInner;
        indices[iIndex++] = vNextInner;
    }
    indexCount_ = (uint32_t)indices.size();

    size_t vertexBufferSize = sizeof(VertexData) * vertices.size();
    vertexResource_ = DxUtils::CreateBufferResource(device, vertexBufferSize);
    VertexData* vData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
    memcpy(vData, vertices.data(), vertexBufferSize);
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = (UINT)vertexBufferSize;
    vbv_.StrideInBytes = sizeof(VertexData);

    size_t indexBufferSize = sizeof(uint32_t) * indices.size();
    indexResource_ = DxUtils::CreateBufferResource(device, indexBufferSize);
    uint32_t* iData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
    memcpy(iData, indices.data(), indexBufferSize);
    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = (UINT)indexBufferSize;
    ibv_.Format = DXGI_FORMAT_R32_UINT;
}

void MeshParticleObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw || numInstance_ == 0) return;

    ID3D12GraphicsCommandList* commandList = sEngine->GetDxCommon()->GetCommandList();
    
    // パイプライン設定を追加
    commandList->SetGraphicsRootSignature(sEngine->GetPSOManager()->GetRootSignature("Particle"));
    commandList->SetPipelineState(sEngine->GetPSOManager()->GetPSO("Particle"));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vbv_);
    commandList->IASetIndexBuffer(&ibv_);

    // レジスタ番号を旧ParticleObjectの仕様（0:インスタンス, 1:マテリアル）に合わせる
    commandList->SetGraphicsRootDescriptorTable(0, instanceSrvHandleGPU_);
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());

    auto lightMgr = LightResource::GetLightManager();
    if (lightMgr) {
        commandList->SetGraphicsRootConstantBufferView(2, lightMgr->GetDirectionalLightGroupAddress());
    }

    commandList->SetGraphicsRootDescriptorTable(3, sTexMgr->GetGpuHandle(textureKey));
    
    commandList->DrawIndexedInstanced(indexCount_, numInstance_, 0, 0, 0);
}

void MeshParticleObject::CreateCylinderMesh() {
    ID3D12Device* device = sEngine->GetDevice();

    const uint32_t kSubdivision = 32;
    const float kRadius = 1.0f;
    const float kHeight = 2.0f;

    uint32_t kVertexCount = (kSubdivision + 1) * 2;
    uint32_t kIndexCount = kSubdivision * 6;

    std::vector<VertexData> vertices(kVertexCount);
    float halfHeight = kHeight / 2.0f;
    float thetaStep = static_cast<float>(3.14159265f * 2.0f / kSubdivision);

    uint32_t vIndex = 0;
    for (uint32_t i = 0; i <= kSubdivision; ++i) {
        float theta = i * thetaStep;
        float c = cosf(theta);
        float s = sinf(theta);
        float u = static_cast<float>(i) / kSubdivision;

        // 下側の頂点 (地面)
        vertices[vIndex].position = { c * kRadius, 0.0f, s * kRadius, 1.0f };
        vertices[vIndex].normal = { c, 0.0f, s };
        vertices[vIndex].texcoord = { u, 0.0f };
        vIndex++;

        // 上側の頂点 (先端)
        vertices[vIndex].position = { c * kRadius, kHeight, s * kRadius, 1.0f };
        vertices[vIndex].normal = { c, 0.0f, s };
        vertices[vIndex].texcoord = { u, 1.0f };
        vIndex++;
    }

    std::vector<uint32_t> indices(kIndexCount);
    uint32_t iIndex = 0;
    for (uint32_t i = 0; i < kSubdivision; ++i) {
        uint32_t current = i * 2;
        uint32_t next = (i + 1) * 2;

        // 時計回り
        indices[iIndex++] = current;
        indices[iIndex++] = current + 1;
        indices[iIndex++] = next;

        indices[iIndex++] = next;
        indices[iIndex++] = current + 1;
        indices[iIndex++] = next + 1;
    }
    indexCount_ = (uint32_t)indices.size();

    size_t vertexBufferSize = sizeof(VertexData) * vertices.size();
    vertexResource_ = DxUtils::CreateBufferResource(device, vertexBufferSize);
    VertexData* vData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
    memcpy(vData, vertices.data(), vertexBufferSize);
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = (UINT)vertexBufferSize;
    vbv_.StrideInBytes = sizeof(VertexData);

    size_t indexBufferSize = sizeof(uint32_t) * indices.size();
    indexResource_ = DxUtils::CreateBufferResource(device, indexBufferSize);
    uint32_t* iData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
    memcpy(iData, indices.data(), indexBufferSize);
    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = (UINT)indexBufferSize;
    ibv_.Format = DXGI_FORMAT_R32_UINT;
}

