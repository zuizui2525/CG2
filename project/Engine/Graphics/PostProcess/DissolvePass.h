#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include "Engine/Math/MathStructs.h"
#include <wrl.h>

/// <summary>
/// ディゾルブポストプロセスパス
/// </summary>
class DissolvePass : public IPostProcessPass {
public:
    DissolvePass() = default;
    ~DissolvePass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータ調整用アクセサ
    void SetThreshold(float threshold);
    float GetThreshold() const;

    void SetEdgeWidth(float width);
    float GetEdgeWidth() const;

    void SetEdgeColor(const Vector3& color);
    Vector3 GetEdgeColor() const;

    // ノイズテクスチャ切り替え用
    void SetActiveNoiseIndex(int32_t index);
    int32_t GetActiveNoiseIndex() const;

private:
    // 定数バッファアライメント（16バイト）
    struct DissolveParams {
        float threshold;   // しきい値
        float edgeWidth;   // 境界線幅
        Vector2 pad;       // パディング
        Vector3 edgeColor; // 境界色
        float pad2;        // パディング
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    DissolveParams* paramsData_ = nullptr;

    bool isActive_ = false;
    int32_t activeNoiseIndex_ = 0;
};
