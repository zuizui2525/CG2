#include "SpriteObject.h"
#include "Zuizui.h"
#include "Camera.h"
#include "TextureManager.h"
#include <imgui.h>

void SpriteObject::SetSize(float width, float height) {
    width_ = width;
    height_ = height;
    UpdateVertexData(); // サイズが変わったら頂点を再計算
}

void SpriteObject::UpdateVertexData() {
    if (vertexData_ == nullptr) return;

    // 座標の設定 (左上原点の場合)
    vertexData_[0] = { {0.0f, height_, 0.0f, 1.0f}, {0,1}, {0,0,-1} }; // 左下
    vertexData_[1] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0,0}, {0,0,-1} };    // 左上
    vertexData_[2] = { {width_, height_, 0.0f, 1.0f}, {1,1}, {0,0,-1} }; // 右下
    vertexData_[3] = { {width_, 0.0f, 0.0f, 1.0f}, {1,0}, {0,0,-1} };    // 右上
}

void SpriteObject::Initialize(int lightingMode) {
    // Material
    materialResource_ = CreateBufferResource(sEngine->GetDevice(), sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 0;
    materialData_->uvtransform = Math::MakeIdentity();
    materialData_->shininess = 30.0f;

    // WVP
    wvpResource_ = CreateBufferResource(sEngine->GetDevice(), sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Vertex
    vertexResource_ = CreateBufferResource(sEngine->GetDevice(), sizeof(VertexData) * 4);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 4;
    vbView_.StrideInBytes = sizeof(VertexData);

    // Mapしてポインタを保存しておく
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // 初期サイズで頂点を設定
    UpdateVertexData();

    // Index
    indexResource_ = CreateBufferResource(sEngine->GetDevice(), sizeof(uint32_t) * 6);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * 6;
    ibView_.Format = DXGI_FORMAT_R32_UINT;
    uint32_t* idx;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idx));
    idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 1; idx[4] = 3; idx[5] = 2;
}

void SpriteObject::Update() {
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, sCamera->GetViewMatrix2D()), sCamera->GetProjectionMatrix2D());
    wvpData_->WVP = wvp;
    wvpData_->world = world;

    Matrix4x4 uv = Math::MakeScaleMatrix(uvTransform_.scale);
    uv = Math::Multiply(uv, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uv = Math::Multiply(uv, Math::MakeTranslateMatrix(uvTransform_.translate));
    materialData_->uvtransform = uv;
}

void SpriteObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw) return;

    sEngine->GetDxCommon()->GetCommandList()->SetGraphicsRootSignature(sEngine->GetPSOManager()->GetRootSignature("Object3D"));
    sEngine->GetDxCommon()->GetCommandList()->SetPipelineState(sEngine->GetPSOManager()->GetPSO("Object3D"));

    sEngine->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vbView_);
    sEngine->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&ibView_);
    sEngine->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    sEngine->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    sEngine->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(2, sCamera->GetGPUVirtualAddress());
    sEngine->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(6, sTexMgr->GetGpuHandle(textureKey));

    sEngine->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void SpriteObject::ImGuiControl(const std::string& name) {
    ImGui::Begin("Sprite List");
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {

            std::string label = "##" + name;

            // --- Sprite特有の設定 (サイズ) ---
            if (ImGui::CollapsingHeader(("Sprite Settings" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                float size[2] = { width_, height_ };
                if (ImGui::DragFloat2(("Size" + label).c_str(), size, 1.0f)) {
                    SetSize(size[0], size[1]);
                }
            }

            // --- 共通のSRT設定 ---
            if (ImGui::CollapsingHeader(("Transform" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3(("Scale" + label).c_str(), &transform_.scale.x, 0.01f);
                ImGui::DragFloat3(("Rotate" + label).c_str(), &transform_.rotate.x, 0.01f);
                ImGui::DragFloat3(("Translate" + label).c_str(), &transform_.translate.x, 1.0f);
            }

            // --- カラー設定 ---
            if (ImGui::CollapsingHeader(("Color" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4(("Color" + label).c_str(), &materialData_->color.x, ImGuiColorEditFlags_AlphaBar);
            }

            // --- UV設定 (Sprite特有) ---
            if (ImGui::CollapsingHeader(("UV Transform" + label).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat2(("uvScale" + label).c_str(), &uvTransform_.scale.x, 0.01f);
                ImGui::DragFloat(("uvRotate" + label).c_str(), &uvTransform_.rotate.z, 0.01f);
                ImGui::DragFloat2(("uvTranslate" + label).c_str(), &uvTransform_.translate.x, 0.01f);
            }
        }
        ImGui::End();
    }
}
