#include "SpriteObject.h"
#include "Camera.h"
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

void SpriteObject::Initialize(ID3D12Device* device) {
    // Material
    materialResource_ = CreateBufferResource(device, sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 0;
    materialData_->uvtransform = Math::MakeIdentity();
    materialData_->shininess = 30.0f;

    // WVP
    wvpResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Vertex
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 4;
    vbView_.StrideInBytes = sizeof(VertexData);

    // Mapしてポインタを保存しておく
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // 初期サイズで頂点を設定
    UpdateVertexData();

    // Index
    indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * 6);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * 6;
    ibView_.Format = DXGI_FORMAT_R32_UINT;
    uint32_t* idx;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idx));
    idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 1; idx[4] = 3; idx[5] = 2;
}

void SpriteObject::Update(const Camera* camera) {
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, camera->GetViewMatrix2D()), camera->GetProjectionMatrix2D());
    wvpData_->WVP = wvp;
    wvpData_->world = world;

    Matrix4x4 uv = Math::MakeScaleMatrix(uvTransform_.scale);
    uv = Math::Multiply(uv, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uv = Math::Multiply(uv, Math::MakeTranslateMatrix(uvTransform_.translate));
    materialData_->uvtransform = uv;
}

void SpriteObject::Draw(ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
    D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
    D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
    ID3D12PipelineState* pipelineState,
    ID3D12RootSignature* rootSignature,
    bool enableDraw) {
    if (!enableDraw) return;

    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);

    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->IASetIndexBuffer(&ibView_);
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, lightAddress);
    commandList->SetGraphicsRootConstantBufferView(3, cameraAddress);
    commandList->SetGraphicsRootDescriptorTable(4, textureHandle);

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void SpriteObject::ImGuiControl() {
    if (ImGui::CollapsingHeader("Sprite Settings")) {
        float size[2] = { width_, height_ };
        if (ImGui::DragFloat2("Size", size, 1.0f)) {
            SetSize(size[0], size[1]);
        }
    }

    if (ImGui::CollapsingHeader("SRT")) {
        ImGui::DragFloat3("Scale", &transform_.scale.x, 0.01f);
        ImGui::DragFloat3("Rotate", &transform_.rotate.x, 0.01f);
        ImGui::DragFloat3("Translate", &transform_.translate.x, 1.0f);
    }
    if (ImGui::CollapsingHeader("Color")) {
        ImGui::ColorEdit4("Color", &materialData_->color.x, true);
    }
    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::RadioButton("None", &materialData_->enableLighting, 0);
        ImGui::RadioButton("Lambert", &materialData_->enableLighting, 1);
        ImGui::RadioButton("HalfLambert", &materialData_->enableLighting, 2);
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader("uvSRT")) {
        ImGui::DragFloat2("uvScale", &uvTransform_.scale.x, 0.01f);
        ImGui::DragFloat("uvRotate", &uvTransform_.rotate.z, 0.01f);
        ImGui::DragFloat2("uvTranslate", &uvTransform_.translate.x, 0.01f);
    }
    ImGui::Separator();
}
