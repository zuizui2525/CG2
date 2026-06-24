#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include "Engine/Math/MathStructs.h"
#include <wrl.h>

/// <summary>
/// テレビ砂嵐ノイズ（アナログノイズ）ポストプロセスパス
/// </summary>
class TVNoisePass : public IPostProcessPass {
public:
    TVNoisePass() = default;
    ~TVNoisePass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータ調整用アクセサ
    void SetNoiseStrength(float strength);
    float GetNoiseStrength() const;

private:
    // 定数バッファアライメント（16バイト）
    struct TVNoiseParams {
        float time;          // 時間
        float noiseStrength; // ノイズ強度 (0.0 〜 1.0)
        Vector2 pad;         // パディング
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    TVNoiseParams* paramsData_ = nullptr;

    bool isActive_ = false;
    float accumTime_ = 0.0f;
};
