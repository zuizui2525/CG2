#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include <wrl.h>

/// <summary>
/// ビネットポストプロセスパス
/// </summary>
class VignettePass : public IPostProcessPass {
public:
    VignettePass() = default;
    ~VignettePass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータアクセサ
    void SetScale(float scale) { if (vignetteData_) { vignetteData_->scale = scale; } }
    float GetScale() const { return vignetteData_ ? vignetteData_->scale : 0.0f; }

    void SetExponent(float exponent) { if (vignetteData_) { vignetteData_->exponent = exponent; } }
    float GetExponent() const { return vignetteData_ ? vignetteData_->exponent : 0.0f; }

private:
    struct VignetteParams {
        float scale;
        float exponent;
        float pad[2]; // 16バイトアライメント
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource_;
    VignetteParams* vignetteData_ = nullptr;

    bool isActive_ = false;
    bool isWindowOpen_ = false;
};
