#include "ModelObject.h"
#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Matrix.h"
#include <cassert>

void ModelObject::Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, TextureManager* texMgr, ModelManager* modelMgr, int lightingMode) {
    Object3D::Initialize(engine, lightingMode);
    camera_ = camera;
    dirLight_ = light;
    texMgr_ = texMgr;
    modelMgr_ = modelMgr;
}

void ModelObject::Update() {
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = camera_->GetViewMatrix3D();
    Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix3D();
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

    assert(!modelKey.empty() && "ModelObject::Draw: modelKeyが空です！");
    assert(!textureKey.empty() && "ModelObject::Draw: textureKeyが空です！");
   
    auto modelData = modelMgr_->GetModelData(modelKey);
    assert(modelData && "ModelObject::Draw: 指定されたmodelKeyがModelManagerに登録されていません！");

    auto commandList = engine_->GetDxCommon()->GetCommandList();
    commandList->SetGraphicsRootSignature(engine_->GetPSOManager()->GetRootSignature("Object3D"));
    commandList->SetPipelineState(engine_->GetPSOManager()->GetPSO("Object3D"));

    // マネージャが持っているVBVを使用
    commandList->IASetVertexBuffers(0, 1, &modelData->vbv);

    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, dirLight_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(3, camera_->GetGPUVirtualAddress());

    // 指定されたキーでテクスチャ取得
    commandList->SetGraphicsRootDescriptorTable(4, texMgr_->GetGpuHandle(textureKey));

    commandList->DrawInstanced((UINT)modelData->vertices.size(), 1, 0, 0);
}
