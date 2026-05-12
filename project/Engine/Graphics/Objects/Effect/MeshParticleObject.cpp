#include "MeshParticleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"

void MeshParticleObject::Initialize(int lightingMode) {
    BaseParticleObject::Initialize(lightingMode);
    CreateCubeMesh();
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
