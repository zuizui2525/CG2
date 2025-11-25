#include "SpriteObject.h"

SpriteObject::SpriteObject(ID3D12Device* device, int width, int height) {
    // Material
    materialResource_ = CreateBufferResource(device, sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 0;
    materialData_->uvtransform = Math::MakeIdentity();

    // WVP
    wvpResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = Math::MakeIdentity();
    wvpData_->world = Math::MakeIdentity();

    // Vertex (四角形)
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 4;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* vtx;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
    vtx[0] = { {0.0f, (float)height, 0.0f, 1.0f}, {0,1}, {0,0,-1} };
    vtx[1] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0,0}, {0,0,-1} };
    vtx[2] = { {(float)width, (float)height, 0.0f, 1.0f}, {1,1}, {0,0,-1} };
    vtx[3] = { {(float)width, 0.0f, 0.0f, 1.0f}, {1,0}, {0,0,-1} };

    // Index
    indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * 6);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * 6;
    ibView_.Format = DXGI_FORMAT_R32_UINT;
    uint32_t* idx;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idx));
    idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 1; idx[4] = 3; idx[5] = 2;
}

SpriteObject::~SpriteObject() {}

void SpriteObject::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, viewMatrix), projectionMatrix);
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
    ID3D12PipelineState* pipelineState,
    ID3D12RootSignature* rootSignature,
    bool enableDraw) {
    if (!enableDraw) return;

    // パイプラインの選択
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);

    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->IASetIndexBuffer(&ibView_);
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, lightAddress);
    commandList->SetGraphicsRootDescriptorTable(3, textureHandle);

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void SpriteObject::ImGuiControl() {
    if (ImGui::CollapsingHeader("SRT")) {
        ImGui::DragFloat3("Scale", &transform_.scale.x, 0.01f); // 球の拡縮を変更するUI
        ImGui::DragFloat3("Rotate", &transform_.rotate.x, 0.01f); // 球の回転を変更するUI
        ImGui::DragFloat3("Translate", &transform_.translate.x, 1.0f); // 球の位置を変更するUI
    }
    if (ImGui::CollapsingHeader("Color")) {
        ImGui::ColorEdit4("Color", &materialData_->color.x, true); // 色の値を変更するUI
    }
    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::RadioButton("None", &materialData_->enableLighting, 0);
        ImGui::RadioButton("Lambert", &materialData_->enableLighting, 1);
        ImGui::RadioButton("HalfLambert", &materialData_->enableLighting, 2);
    }
    ImGui::Separator();
    ImGui::Text("uvTransform");
    if (ImGui::CollapsingHeader("uvSRT")) {
        ImGui::DragFloat2("uvScale", &uvTransform_.scale.x, 0.01f); // uv球の拡縮を変更するUI
        ImGui::DragFloat("uvRotate", &uvTransform_.rotate.z, 0.01f); // uv球の回転を変更するUI
        ImGui::DragFloat2("uvTranslate", &uvTransform_.translate.x, 0.01f); // uv球の位置を変更するUI
    }
    ImGui::Separator();
}
