#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include <wrl.h>

/// <summary>
/// グレースケールポストプロセスパス
/// </summary>
class GrayscalePass : public IPostProcessPass {
public:
    GrayscalePass() = default;
    ~GrayscalePass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override {}

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    bool isActive_ = false;
};
