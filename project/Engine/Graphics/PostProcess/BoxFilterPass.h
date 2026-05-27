#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include <wrl.h>

/// <summary>
/// BoxFilter (Smoothing) ポストプロセスパス
/// </summary>
class BoxFilterPass : public IPostProcessPass {
public:
    BoxFilterPass() = default;
    ~BoxFilterPass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータアクセサ
    void SetKernelRadius(int32_t radius) { if (paramsData_) { paramsData_->kernelRadius = radius; } }
    int32_t GetKernelRadius() const { return paramsData_ ? paramsData_->kernelRadius : 1; }

private:
    struct BoxFilterParams {
        int32_t kernelRadius; // k (1: 3x3, 2: 5x5)
        float pad[3];        // 16バイトアライメント用
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    BoxFilterParams* paramsData_ = nullptr;

    bool isActive_ = false;
};
