#include "TriangleObject.h"
#include "Function.h"
#include "Zuizui.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "LightManager.h"
#include "TextureManager.h"

void TriangleObject::Initialize(int lightingMode) {
    // 基底クラスの初期化
    Object3D::Initialize(lightingMode);
    
    // Vertex (三角形3頂点)
    vertexResource_ = CreateBufferResource(sEngine->GetDevice(), sizeof(VertexData) * 3);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * 3;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* vtx;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
    vtx[0].position = { -0.5f, -0.5f, 0.0f, 1.0f }; // 左下
    vtx[0].texcoord = { 0.0f, 1.0f };
    vtx[0].normal = { 0.0f, 0.0f, -1.0f };

    vtx[1].position = { 0.0f, 0.5f, 0.0f, 1.0f }; // 上
    vtx[1].texcoord = { 0.5f, 0.0f };
    vtx[1].normal = { 0.0f, 0.0f, -1.0f };

    vtx[2].position = { 0.5f, -0.5f, 0.0f, 1.0f }; // 右下
    vtx[2].texcoord = { 1.0f, 1.0f };
    vtx[2].normal = { 0.0f, 0.0f, -1.0f };
}

void TriangleObject::Update() {
    Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, sCamera->GetViewMatrix3D()), sCamera->GetProjectionMatrix3D());
    Matrix4x4 worldForNormal = world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;
    wvpData_->WVP = wvp;
    wvpData_->world = world;
    wvpData_->WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    Matrix4x4 uv = Math::MakeScaleMatrix(uvTransform_.scale);
    uv = Math::Multiply(uv, Math::MakeRotateZMatrix(uvTransform_.rotate.z));
    uv = Math::Multiply(uv, Math::MakeTranslateMatrix(uvTransform_.translate));
    materialData_->uvtransform = uv;
}

void TriangleObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw) return;
    // コマンドリスト
    auto commandList = EngineResource::GetEngine()->GetDxCommon()->GetCommandList();
    // パイプラインの選択
    commandList->SetGraphicsRootSignature(EngineResource::GetEngine()->GetPSOManager()->GetRootSignature("Object3D"));
    commandList->SetPipelineState(EngineResource::GetEngine()->GetPSOManager()->GetPSO("Object3D"));
    // VBV設定
    commandList->IASetVertexBuffers(0, 1, &vbView_);
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
    commandList->DrawInstanced(3, 1, 0, 0);
}
