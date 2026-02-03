#pragma once
#include <map>
#include <string>
#include <memory>
#include <d3d12.h>
#include <wrl.h>
#include "BaseCamera.h"

class CameraManager {
public:
    struct CameraForGPU {
        Matrix4x4 view;
        Matrix4x4 projection;
        Vector3 worldPosition;
    };

    void Initialize(ID3D12Device* device);
    void Update();

    // --- カメラ管理 ---
    void AddCamera(const std::string& name, std::shared_ptr<BaseCamera> camera);
    void SetActiveCamera(const std::string& name);
    BaseCamera* GetActiveCamera() { return activeCamera_; }

    // --- 3D行列ゲッター (アクティブカメラから取得) ---
    const Matrix4x4& GetViewMatrix3D() const { return activeCamera_->GetViewMatrix(); }
    const Matrix4x4& GetProjectionMatrix3D() const { return activeCamera_->GetProjectionMatrix(); }

    // --- 2D行列ゲッター ---
    const Matrix4x4& GetViewMatrix2D() const { return viewMatrix2D_; }
    const Matrix4x4& GetProjectionMatrix2D() const { return projectionMatrix2D_; }

    // GPUアドレス取得
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return resource_->GetGPUVirtualAddress(); }

private:
    std::map<std::string, std::shared_ptr<BaseCamera>> cameras_;
    BaseCamera* activeCamera_ = nullptr;

    // 2D用行列
    Matrix4x4 viewMatrix2D_;
    Matrix4x4 projectionMatrix2D_;

    // 定数バッファ関連
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    CameraForGPU* data_ = nullptr;
};
