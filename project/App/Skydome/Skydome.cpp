#include "Skydome.h"

void Skydome::Initialize(DxCommon* dxCommon, TextureManager* textureManager) {
    // モデルの生成（PlaySceneから移行）
    model_ = std::make_unique<ModelObject>(dxCommon->GetDevice(), "resources/AL/skydome/skydome.obj", Vector3{ 0.0f, 0.0f, 0.0f }, 0);
    // テクスチャのロード
    textureManager->LoadTexture("skydome", model_->GetModelData()->material.textureFilePath);
}

void Skydome::Update(Camera* camera) {
    rotate_.y += 0.003f;
    
    model_->SetRotate(rotate_);
    model_->Update(camera);
}

void Skydome::Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight) {
    model_->Draw(
        dxCommon->GetCommandList(),
        textureManager->GetGpuHandle("skydome"),
        dirLight->GetGPUVirtualAddress(),
        psoManager->GetPSO("Object3D"),
        psoManager->GetRootSignature("Object3D"),
        true
    );
}
