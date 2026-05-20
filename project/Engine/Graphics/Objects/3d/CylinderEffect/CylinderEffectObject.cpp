#include "Engine/Graphics/Objects/3d/CylinderEffect/CylinderEffectObject.h"
#include "Engine/Base/Utils/DxUtils.h"
#include "Engine/Zuizui.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Math/Matrix/Matrix.h"
#include <numbers>
#include <cmath>
#include <vector>

CylinderEffectObject::CylinderEffectObject(uint32_t subdivision, float radius, float height)
    : subdivision_(subdivision), radius_(radius), height_(height) {
}

void CylinderEffectObject::Initialize(int lightingMode) {
    Object3D::Initialize(lightingMode);
    CreateMesh();
}

void CylinderEffectObject::Update() {
    if (needsUpdate_) {
        CreateMesh();
        needsUpdate_ = false;
    }

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

void CylinderEffectObject::Draw(const std::string& textureKey, const std::string& envMapKey) {
    if (!isVisible_) return;
    auto commandList = EngineResource::GetEngine()->GetDxCommon()->GetCommandList();
    commandList->SetGraphicsRootSignature(EngineResource::GetEngine()->GetPSOManager()->GetRootSignature("Object3D"));
    commandList->SetPipelineState(EngineResource::GetEngine()->GetPSOManager()->GetPSO("Object3D"));
    commandList->IASetVertexBuffers(0, 1, &vbView_);
    commandList->IASetIndexBuffer(&ibView_);
    commandList->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, CameraResource::GetCameraManager()->GetGPUVirtualAddress());
    auto lightMgr = LightResource::GetLightManager();
    if (lightMgr) {
        commandList->SetGraphicsRootConstantBufferView(3, lightMgr->GetDirectionalLightGroupAddress());
        commandList->SetGraphicsRootConstantBufferView(4, lightMgr->GetPointLightGroupAddress());
        commandList->SetGraphicsRootConstantBufferView(5, lightMgr->GetSpotLightGroupAddress());
    }
    commandList->SetGraphicsRootDescriptorTable(6, sTexMgr->GetGpuHandle(textureKey));
    
    if (!envMapKey.empty()) {
        commandList->SetGraphicsRootDescriptorTable(7, sTexMgr->GetGpuHandle(envMapKey));
    } else {
        commandList->SetGraphicsRootDescriptorTable(7, sTexMgr->GetGpuHandle("skyboxTex")); 
    }
    
    commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void CylinderEffectObject::CreateMesh() {
    // 側面のみの頂点数: (分割数 + 1) * 2
    uint32_t kVertexCount = (subdivision_ + 1) * 2;
    uint32_t kIndexCount = subdivision_ * 6;

    vertexResource_ = DxUtils::CreateBufferResource(sEngine->GetDevice(), sizeof(VertexData) * kVertexCount);
    vbView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
    vbView_.StrideInBytes = sizeof(VertexData);

    VertexData* vtx;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));

    float halfHeight = height_ / 2.0f;
    float thetaStep = static_cast<float>(3.14159265f * 2.0f / subdivision_);

    uint32_t vIndex = 0;
    for (uint32_t i = 0; i <= subdivision_; ++i) {
        float theta = i * thetaStep;
        float c = cosf(theta);
        float s = sinf(theta);
        float x = c * radius_;
        float z = s * radius_;
        float u = static_cast<float>(i) / subdivision_;

        // 下側の頂点 (地面)
        vtx[vIndex].position = { x, 0.0f, z, 1.0f };
        vtx[vIndex].normal = { c, 0.0f, s };
        vtx[vIndex].texcoord = { u, 0.0f };
        vIndex++;

        // 上側の頂点 (先端)
        vtx[vIndex].position = { x, height_, z, 1.0f };
        vtx[vIndex].normal = { c, 0.0f, s };
        vtx[vIndex].texcoord = { u, 1.0f };
        vIndex++;
    }

    vertexResource_->Unmap(0, nullptr);

    indexResource_ = DxUtils::CreateBufferResource(sEngine->GetDevice(), sizeof(uint32_t) * kIndexCount);
    ibView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
    ibView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* idxGPU = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idxGPU));

    uint32_t iIndex = 0;
    for (uint32_t i = 0; i < subdivision_; ++i) {
        uint32_t current = i * 2;
        uint32_t next = (i + 1) * 2;

        // 表面 (時計回り)
        // 三角形1 (下、上、次の下)
        idxGPU[iIndex++] = current;
        idxGPU[iIndex++] = current + 1;
        idxGPU[iIndex++] = next;

        // 三角形2 (次の下、上、次の上)
        idxGPU[iIndex++] = next;
        idxGPU[iIndex++] = current + 1;
        idxGPU[iIndex++] = next + 1;
    }
    indexCount_ = kIndexCount;

    indexResource_->Unmap(0, nullptr);
}
