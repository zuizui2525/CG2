#include "Player.h"

void Player::Initialize(DxCommon* dxCommon, TextureManager* textureManager, Input* input) {
    input_ = input;
    // ポジション
    position_ = { 0.2f,0.2f,0.0f };
    // モデルの生成（PlaySceneから移行）
    model_ = std::make_unique<ModelObject>(dxCommon->GetDevice(), "resources/AL/player/player.obj", position_);
    // テクスチャのロード
    textureManager->LoadTexture("player", model_->GetModelData()->material.textureFilePath);
}

void Player::Update(Camera* camera) {

    if (input_->Press(DIK_W)) {
        position_.y += speed_;
    } else if (input_->Press(DIK_S)) {
        position_.y -= speed_;
    } else if (input_->Press(DIK_A)) {
        position_.x -= speed_;
    } else if (input_->Press(DIK_D)) {
        position_.x += speed_;
    }
    model_->SetScale(Vector3{ 0.1f, 0.1f, 0.1f });
    model_->SetPosition(position_);

    model_->Update(camera);
}

void Player::Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight) {
    model_->Draw(
        dxCommon->GetCommandList(),
        textureManager->GetGpuHandle("player"),
        dirLight->GetGPUVirtualAddress(),
        psoManager->GetPSO("Object3D"),
        psoManager->GetRootSignature("Object3D"),
        true
    );
}
