#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include "Engine/Math/MathStructs.h"
#include <wrl.h>

/// <summary>
/// ラジアルブラーポストプロセスパス
/// </summary>
class RadialBlurPass : public IPostProcessPass {
public:
    RadialBlurPass() = default;
    ~RadialBlurPass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータアクセサ
    void SetParams(const Vector2& center, float blurWidth) {
        if (paramsData_) {
            paramsData_->center = center;
            paramsData_->blurWidth = blurWidth;
        }
    }
    Vector2 GetCenter() const { return paramsData_ ? paramsData_->center : Vector2{0.5f, 0.5f}; }
    float GetBlurWidth() const { return paramsData_ ? paramsData_->blurWidth : 0.0f; }

private:
    struct RadialBlurParams {
        Vector2 center;
        float blurWidth;
        float pad; // 16byte alignment
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    RadialBlurParams* paramsData_ = nullptr;

    bool isActive_ = false;
};
