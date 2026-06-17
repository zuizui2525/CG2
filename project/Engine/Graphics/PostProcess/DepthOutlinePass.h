#pragma once
#include "Engine/Graphics/PostProcess/IPostProcessPass.h"
#include "Engine/Graphics/PSO/Shader/ShaderProgram.h"
#include "Engine/Math/Matrix/Matrix.h"
#include <wrl.h>

/// <summary>
/// 深度（Depth）ベースのエッジ検出ポストプロセスパス
/// </summary>
class DepthOutlinePass : public IPostProcessPass {
public:
    DepthOutlinePass() = default;
    ~DepthOutlinePass() override = default;

    void Initialize(ID3D12Device* device) override;
    void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) override;
    void ImGuiControl() override;

    bool IsActive() const override { return isActive_; }
    void SetActive(bool active) override { isActive_ = active; }

    // パラメータ調整用アクセサ
    void SetEdgeWidth(float width) { if (paramsData_) { paramsData_->edgeWidth = width; } }
    float GetEdgeWidth() const { return paramsData_ ? paramsData_->edgeWidth : 1.0f; }

    void SetThreshold(float threshold) { if (paramsData_) { paramsData_->threshold = threshold; } }
    float GetThreshold() const { return paramsData_ ? paramsData_->threshold : 0.1f; }

    void SetScale(float scale) { if (paramsData_) { paramsData_->scale = scale; } }
    float GetScale() const { return paramsData_ ? paramsData_->scale : 6.0f; }

    void SetEdgeColor(const Vector3& color) {
        if (paramsData_) {
            paramsData_->edgeColor = color;
        }
    }
    Vector3 GetEdgeColor() const { return paramsData_ ? paramsData_->edgeColor : Vector3{0.0f, 0.0f, 0.0f}; }

private:
    // 定数バッファアライメント(16バイト)
    struct DepthOutlineParams {
        Matrix4x4 projectionInverse;
        Vector3 edgeColor;
        float edgeWidth;
        float threshold;
        float scale;
        float pad[2]; // 16バイトアライメント用
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    ShaderProgram shaderProgram_;

    // 定数バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> paramsResource_;
    DepthOutlineParams* paramsData_ = nullptr;

    // 深度テクスチャ用のSRVディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE depthSrvCpu_{};
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGpu_{};
    ID3D12Resource* lastDepthResource_ = nullptr;

    bool isActive_ = false;
};
