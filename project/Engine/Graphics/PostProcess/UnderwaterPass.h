#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include "Engine/Math/MathStructs.h"
#include <wrl.h>

/// <summary>
/// Underwater post-process pass class (Distortion, Gaussian Blur & Optional Color Tint)
/// </summary>
class UnderwaterPass : public IPostProcessPass {
public:
    UnderwaterPass() = default;
    ~UnderwaterPass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // Parameter Accessors
    void SetDistortionStrength(float strength) { if (paramsData_) { paramsData_->distortionStrength = strength; } }
    float GetDistortionStrength() const { return paramsData_ ? paramsData_->distortionStrength : 0.0f; }

    void SetDistortionFrequency(float freq) { if (paramsData_) { paramsData_->distortionFrequency = freq; } }
    float GetDistortionFrequency() const { return paramsData_ ? paramsData_->distortionFrequency : 0.0f; }

    void SetBlurStrength(float strength) { if (paramsData_) { paramsData_->blurStrength = strength; } }
    float GetBlurStrength() const { return paramsData_ ? paramsData_->blurStrength : 0.0f; }

    void SetBlurWeight(float weight) { if (paramsData_) { paramsData_->blurWeight = weight; } }
    float GetBlurWeight() const { return paramsData_ ? paramsData_->blurWeight : 0.0f; }

    void SetWaterColorIntensity(float intensity) { if (paramsData_) { paramsData_->waterColorIntensity = intensity; } }
    float GetWaterColorIntensity() const { return paramsData_ ? paramsData_->waterColorIntensity : 0.0f; }

    void SetWaterColor(const Vector3& color) { if (paramsData_) { paramsData_->waterColor = color; } }
    Vector3 GetWaterColor() const { return paramsData_ ? paramsData_->waterColor : Vector3{0.0f, 0.0f, 0.0f}; }

private:
    struct UnderwaterParams {
        float time;
        float distortionStrength;
        float distortionFrequency;
        float blurStrength;
        float blurWeight;
        float waterColorIntensity;
        Vector2 pad; // 8 bytes padding for alignment
        Vector3 waterColor;
        float pad2; // 4 bytes padding for alignment
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // Constant Buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    UnderwaterParams* paramsData_ = nullptr;

    bool isActive_ = false;
    float accumTime_ = 0.0f;
};
