#include "ModelObject.h"
#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"
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

    auto commandList = sEngine->GetDxCommon()->GetCommandList();
    commandList->SetGraphicsRootSignature(sEngine->GetPSOManager()->GetRootSignature("Object3D"));
    commandList->SetPipelineState(sEngine->GetPSOManager()->GetPSO("Object3D"));

    // マネージャが持っているVBVを使用
    commandList->IASetVertexBuffers(0, 1, &modelData->vbv);

    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, sCamera->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(3, sDirLight->GetGPUVirtualAddress());
    if (sPointLight) {
        commandList->SetGraphicsRootConstantBufferView(4, sPointLight->GetGPUVirtualAddress());
    }

    // 指定されたキーでテクスチャ取得
    commandList->SetGraphicsRootDescriptorTable(5, sTexMgr->GetGpuHandle(textureKey));

    commandList->DrawInstanced((UINT)modelData->vertices.size(), 1, 0, 0);
}
