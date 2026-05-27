#pragma once
#include <d3d12.h>
#include <memory>
#include "Engine/Graphics/Texture/RenderTexture.h"
#include "Engine/Math/MathStructs.h"

// 前方宣言
class PSOManager;

enum class PostEffectMode : int32_t {
    None = 0,      // ポストエフェクトなし（元々の青背景 ＋ 等倍コピー）
    Red = 1,       // 赤（デバッグ用赤背景 ＋ 等倍コピー）
    Black = 2,     // 黒（デバッグ用黒背景 ＋ 等倍コピー）
    Grayscale = 3, // グレースケール（黒背景 ＋ グレースケールフィルタ）
    Sepia = 4,      // セピア調（黒背景 ＋ セピア調フィルタ）
    Vignette = 5   // ビネット（黒背景 ＋ ビネットフィルタ）
};

enum class PostClearColorMode : int32_t {
    Blue = 0,  // 元々の青背景
    Red = 1,   // デバッグ赤
    Black = 2  // デバッグ黒
};

struct PostProcessParams {
    int32_t enableGrayscale;
    int32_t enableSepia;
    int32_t enableVignette;
    float vignetteScale;
    float vignetteExponent;
    float pad[3]; // 16バイト境界アライメント用パディング
};

/// <summary>
/// ポストプロセス管理クラス
/// </summary>
class PostProcess {
public:
    PostProcess() = default;
    ~PostProcess() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 描画前処理（レンダーテクスチャを描画ターゲットに設定してクリア）
    /// </summary>
    void PreDraw();

    /// <summary>
    /// 描画後処理（レンダーテクスチャのリソース状態をピクセルシェーダ読み込み用にバリア遷移）
    /// </summary>
    void PostDraw();

    /// <summary>
    /// スワップチェーン（最終画面）への全画面コピー描画
    /// </summary>
    void Draw();

    /// <summary>
    /// ImGui制御（重ね掛けエフェクトやビネットパラメータ調整用）
    /// </summary>
    void ImGuiControl();

    /// <summary>
    /// モード設定（後方互換用。クリアカラーと個別エフェクトを統合設定します）
    /// </summary>
    void SetEffectMode(PostEffectMode mode);

    /// <summary>
    /// 現在のモードを取得（後方互換用）
    /// </summary>
    PostEffectMode GetEffectMode() const { return currentMode_; }

    /// <summary>
    /// 背景クリアカラーモード設定
    /// </summary>
    void SetClearColorMode(PostClearColorMode mode);

    /// <summary>
    /// 現在の背景クリアカラーモードを取得
    /// </summary>
    PostClearColorMode GetClearColorMode() const { return clearColorMode_; }

    /// <summary>
    /// デバッグクリアカラー（赤）の有効無効設定（後方互換用）
    /// </summary>
    void SetDebugClearColor(bool enable) { SetClearColorMode(enable ? PostClearColorMode::Red : PostClearColorMode::Blue); }

    /// <summary>
    /// デバッグクリアカラーが有効かどうかを取得（後方互換用）
    /// </summary>
    bool IsDebugClearColor() const { return clearColorMode_ == PostClearColorMode::Red; }

    // エフェクト設定用のアクセサ
    void SetGrayscaleActive(bool active) { if (postProcessData_) { postProcessData_->enableGrayscale = active ? 1 : 0; } }
    bool IsGrayscaleActive() const { return postProcessData_ ? (postProcessData_->enableGrayscale != 0) : false; }

    void SetSepiaActive(bool active) { if (postProcessData_) { postProcessData_->enableSepia = active ? 1 : 0; } }
    bool IsSepiaActive() const { return postProcessData_ ? (postProcessData_->enableSepia != 0) : false; }

    void SetVignetteActive(bool active) { if (postProcessData_) { postProcessData_->enableVignette = active ? 1 : 0; } }
    bool IsVignetteActive() const { return postProcessData_ ? (postProcessData_->enableVignette != 0) : false; }

    void SetVignetteScale(float scale) { if (postProcessData_) { postProcessData_->vignetteScale = scale; } }
    float GetVignetteScale() const { return postProcessData_ ? postProcessData_->vignetteScale : 0.0f; }

    void SetVignetteExponent(float exponent) { if (postProcessData_) { postProcessData_->vignetteExponent = exponent; } }
    float GetVignetteExponent() const { return postProcessData_ ? postProcessData_->vignetteExponent : 0.0f; }

private:
    std::unique_ptr<RenderTexture> renderTexture_;
    PostEffectMode currentMode_ = PostEffectMode::None; // 互換用保持
    PostClearColorMode clearColorMode_ = PostClearColorMode::Blue;

    // 統合ポストプロセスパラメータ用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> postProcessResource_;
    PostProcessParams* postProcessData_ = nullptr;
    bool isVignetteWindowOpen_ = false;
};
