#pragma once
#include <d3d12.h>
#include <wrl.h>

/// <summary>
/// ポストプロセスエフェクト各パスの共通インターフェース
/// </summary>
class IPostProcessPass {
public:
    virtual ~IPostProcessPass() = default;

    /// <summary>
    /// 初期化（PSO生成やリソース作成）
    /// </summary>
    virtual void Initialize(ID3D12Device* device) = 0;

    /// <summary>
    /// 描画実行
    /// </summary>
    /// <param name="commandList">コマンドリスト</param>
    /// <param name="inputSRV">入力元テクスチャのGPUハンドル（SRV）</param>
    virtual void Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) = 0;

    /// <summary>
    /// ImGui制御用パラメータ調整表示
    /// </summary>
    virtual void ImGuiControl() = 0;

    /// <summary>
    /// パスの有効/無効制御
    /// </summary>
    virtual bool IsActive() const = 0;
    virtual void SetActive(bool active) = 0;
};
