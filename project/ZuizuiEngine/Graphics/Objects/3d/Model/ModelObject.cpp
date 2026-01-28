#include "ModelObject.h"
#include "Zuizui.h"
#include "Camera.h"
#include "LightManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Matrix.h"
#include <cassert>

void ModelObject::Initialize(int lightingMode) {
    Object3D::Initialize(lightingMode);
}

void ModelObject::Update() {
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = sCamera->GetViewMatrix3D();
    Matrix4x4 projectionMatrix = sCamera->GetProjectionMatrix3D();
    Matrix4x4 wvpMatrix = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix, projectionMatrix));

    // 法線用行列（逆転置）
    Matrix4x4 worldForNormal = worldMatrix;
    worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f; worldForNormal.m[3][2] = 0.0f;

    wvpData_->WVP = wvpMatrix;
    wvpData_->world = worldMatrix;
    wvpData_->WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
}

void ModelObject::Draw(const std::string& modelKey, const std::string& textureKey, bool draw) {
    if (!draw) return;

    assert(!modelKey.empty());
    assert(!textureKey.empty());
   
    auto modelData = sModelMgr->GetModelData(modelKey);
    assert(modelData);
    // コマンドリスト
    auto commandList = EngineResource::GetEngine()->GetDxCommon()->GetCommandList();
    // パイプラインの選択
    commandList->SetGraphicsRootSignature(EngineResource::GetEngine()->GetPSOManager()->GetRootSignature("Object3D"));
    commandList->SetPipelineState(EngineResource::GetEngine()->GetPSOManager()->GetPSO("Object3D"));
    // VBV設定
    commandList->IASetVertexBuffers(0, 1, &modelData->vbv);
    // 定数バッファ設定
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, sCamera->GetGPUVirtualAddress());
    auto lightMgr = LightResource::GetLightManager();
    if (lightMgr) {
        commandList->SetGraphicsRootConstantBufferView(3, lightMgr->GetDirectionalLightGroupAddress());
        commandList->SetGraphicsRootConstantBufferView(4, lightMgr->GetPointLightGroupAddress());
        commandList->SetGraphicsRootConstantBufferView(5, lightMgr->GetSpotLightGroupAddress());
    }
    // 指定されたキーでテクスチャ取得
    commandList->SetGraphicsRootDescriptorTable(6, sTexMgr->GetGpuHandle(textureKey));
    // DrawInstanced
    commandList->DrawInstanced((UINT)modelData->vertices.size(), 1, 0, 0);
}
