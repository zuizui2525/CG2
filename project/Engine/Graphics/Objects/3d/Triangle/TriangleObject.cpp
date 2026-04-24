#include "Engine/Graphics/Objects/3d/Triangle/TriangleObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Directional/DirectionalLight.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"

void TriangleObject::Initialize(int lightingMode) {
    // 基底クラスの初期化
    Object3D::Initialize(lightingMode);
    
    // Vertex (三角形3頂点)
    vertexResource_ = DxUtils::CreateBufferResource(sEngine->GetDevice(), sizeof(VertexData) * 3);
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
    Matrix4x4 wvp = Math::Multiply(Math::Multiply(world, CameraResource::GetCameraManager()->GetViewMatrix3D()), CameraResource::GetCameraManager()->GetProjectionMatrix3D());
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

void TriangleObject::Draw(const std::string& textureKey, const std::string& envMapKey, bool draw) {
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
    commandList->SetGraphicsRootConstantBufferView(2, CameraResource::GetCameraManager()->GetGPUVirtualAddress());
    auto lightMgr = LightResource::GetLightManager();
    if (lightMgr) {
        commandList->SetGraphicsRootConstantBufferView(3, lightMgr->GetDirectionalLightGroupAddress());
        commandList->SetGraphicsRootConstantBufferView(4, lightMgr->GetPointLightGroupAddress());
        commandList->SetGraphicsRootConstantBufferView(5, lightMgr->GetSpotLightGroupAddress());
    }
    // 指定されたキーでテクスチャ取得
    commandList->SetGraphicsRootDescriptorTable(6, sTexMgr->GetGpuHandle(textureKey));

    // 環境マップテクスチャ
    if (!envMapKey.empty()) {
        materialData_->environmentCoefficient = 1.0f;
        commandList->SetGraphicsRootDescriptorTable(7, sTexMgr->GetGpuHandle(envMapKey));
    } else {
        materialData_->environmentCoefficient = 0.0f;
        // TextureCube以外のテクスチャを渡すとエラーになるため、空のときはskyboxTexをダミーとして渡す
        commandList->SetGraphicsRootDescriptorTable(7, sTexMgr->GetGpuHandle("skyboxTex")); 
    }
    // DrawInstanced
    commandList->DrawInstanced(3, 1, 0, 0);
}
