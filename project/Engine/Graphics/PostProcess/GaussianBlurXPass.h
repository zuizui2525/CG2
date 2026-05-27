#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include <wrl.h>

/// <summary>
/// 横方向 (X) ガウシアンぼかしポストプロセスパス
/// </summary>
class GaussianBlurXPass : public IPostProcessPass {
public:
    GaussianBlurXPass() = default;
    ~GaussianBlurXPass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override {} // 外部で一元管理するため空にします

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータアクセサ
    void SetParams(int32_t radius, float sigma) {
        if (paramsData_) {
            paramsData_->kernelRadius = radius;
            paramsData_->sigma = sigma;
        }
    }
    int32_t GetKernelRadius() const { return paramsData_ ? paramsData_->kernelRadius : 1; }
    float GetSigma() const { return paramsData_ ? paramsData_->sigma : 2.0f; }

private:
    struct GaussianParams {
        int32_t kernelRadius; // k
        float sigma;          // σ
        float pad[2];         // 16バイトアライメント
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    GaussianParams* paramsData_ = nullptr;

    bool isActive_ = false;
};
