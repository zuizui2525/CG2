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
    Sepia = 4      // セピア調（黒背景 ＋ セピア調フィルタ）
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
    /// モード設定
    /// </summary>
    void SetEffectMode(PostEffectMode mode);

    /// <summary>
    /// 現在のモードを取得
    /// </summary>
    PostEffectMode GetEffectMode() const { return currentMode_; }

    /// <summary>
    /// デバッグクリアカラー（赤）の有効無効設定（後方互換用）
    /// </summary>
    void SetDebugClearColor(bool enable) { SetEffectMode(enable ? PostEffectMode::Red : PostEffectMode::None); }

    /// <summary>
    /// デバッグクリアカラーが有効かどうかを取得（後方互換用）
    /// </summary>
    bool IsDebugClearColor() const { return currentMode_ == PostEffectMode::Red; }

private:
    std::unique_ptr<RenderTexture> renderTexture_;
    PostEffectMode currentMode_ = PostEffectMode::None;
};
