#include "ModelObject.h"
#include "ModelManager.h"
#include "Function.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "TextureManager.h"

void ModelObject::Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, TextureManager* texture, const std::string& filename, int lightingMode) {
    // 基底クラスの初期化
    Object3D::Initialize(engine, lightingMode);
    camera_ = camera;
    dirLight_ = light;
    texture_ = texture;

    // モデルデータ読み込み
    modelData_ = ModelManager::GetInstance().LoadModel(engine_->GetDevice(), filename);

    // 頂点リソース作成
    vertexResource_ = CreateBufferResource(engine_->GetDevice(), sizeof(VertexData) * modelData_->vertices.size());
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = sizeof(VertexData) * (UINT)modelData_->vertices.size();
    vbv_.StrideInBytes = sizeof(VertexData);

    // 頂点データ転送
    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    memcpy(vertexData, modelData_->vertices.data(), sizeof(VertexData) * modelData_->vertices.size());
}

void ModelObject::Update() {
    // ワールド行列
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );

    // WVP行列
    Matrix4x4 worldViewProjection = Math::Multiply(Math::Multiply(worldMatrix, camera_->GetViewMatrix3D()), camera_->GetProjectionMatrix3D());

    // 逆転置行列
    Matrix4x4 worldForNormal = worldViewProjection;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    // 定数バッファに書き込み
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = worldViewProjection;
    wvpData_->world = worldMatrix;
    wvpData_->WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
}

void ModelObject::Draw(const std::string& textureKey, bool draw) {
    if (!draw) return;
    // パイプラインの選択
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootSignature(engine_->GetPSOManager()->GetRootSignature("Object3D"));
    engine_->GetDxCommon()->GetCommandList()->SetPipelineState(engine_->GetPSOManager()->GetPSO("Object3D"));
    
    // VBV設定
    engine_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vbv_);

    // 定数バッファ設定
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(2, dirLight_->GetGPUVirtualAddress());
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(3, camera_->GetGPUVirtualAddress());
    engine_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(4, texture_->GetGpuHandle(textureKey));

    // 描画
    if (draw) {
        engine_->GetDxCommon()->GetCommandList()->DrawInstanced(UINT(modelData_->vertices.size()), 1, 0, 0);
    }
}
