#include "SpriteParticleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"

void SpriteParticleObject::Initialize(int lightingMode) {
    BaseParticleObject::Initialize(lightingMode);
    ID3D12Device* device = sEngine->GetDevice();

    // 板ポリゴンの頂点データ
    vertices_ = {
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}
    };

    size_t vertexBufferSize = sizeof(VertexData) * vertices_.size();
    vertexResource_ = DxUtils::CreateBufferResource(device, vertexBufferSize);
    
    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    memcpy(vertexData, vertices_.data(), vertexBufferSize);

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = (UINT)vertexBufferSize;
    vbv_.StrideInBytes = sizeof(VertexData);
}

void SpriteParticleObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw || numInstance_ == 0) return;

    ID3D12GraphicsCommandList* commandList = sEngine->GetDxCommon()->GetCommandList();
    
    // パイプライン設定を追加
    commandList->SetGraphicsRootSignature(sEngine->GetPSOManager()->GetRootSignature("Particle"));
    commandList->SetPipelineState(sEngine->GetPSOManager()->GetPSO("Particle"));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vbv_);

    // レジスタ番号を旧ParticleObjectの仕様（0:インスタンス, 1:マテリアル）に合わせる
    commandList->SetGraphicsRootDescriptorTable(0, instanceSrvHandleGPU_);
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());

    auto lightMgr = LightResource::GetLightManager();
    if (lightMgr) {
        commandList->SetGraphicsRootConstantBufferView(2, lightMgr->GetDirectionalLightGroupAddress());
    }

    commandList->SetGraphicsRootDescriptorTable(3, sTexMgr->GetGpuHandle(textureKey));
    
    commandList->DrawInstanced((UINT)vertices_.size(), numInstance_, 0, 0);
}
