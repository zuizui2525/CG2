#include "Clear.h"

void Clear::Initialize(DxCommon* dxCommon, TextureManager* textureManager, const Vector3& position) {
    position_ = position;

    // ゴールのモデルを生成 (パスは環境に合わせて変更してください)
    model_ = std::make_unique<ModelObject>(dxCommon->GetDevice(), "resources/AL/goal/goal.obj", position_);

    // テクスチャのロード
    textureManager->LoadTexture("goal", "resources/AL/goal/goal.png");
}

void Clear::Update(Camera* camera) {
    // ゴールをくるくる回す演出
    rotation_.y += 0.03f;

    model_->SetPosition(position_);
    model_->SetRotate(rotation_);
    model_->SetScale({ 0.1f, 0.1f, 0.1f }); // サイズ調整

    model_->Update(camera);
}

void Clear::Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight) {
    model_->Draw(
        dxCommon->GetCommandList(),
        textureManager->GetGpuHandle("goal"),
        dirLight->GetGPUVirtualAddress(),
        psoManager->GetPSO("Object3D"),
        psoManager->GetRootSignature("Object3D"),
        true
    );
}

AABB Clear::GetAABB() const {
    AABB aabb;
    aabb.min = { position_.x - kWidth, position_.y - kHeight, position_.z - kWidth };
    aabb.max = { position_.x + kWidth, position_.y + kHeight, position_.z + kWidth };
    return aabb;
}
