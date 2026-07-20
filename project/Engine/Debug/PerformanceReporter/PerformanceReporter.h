#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <deque>
#include <chrono>

/**
 * @brief パフォーマンス低下（FPSスパイク）やロード遅延を検知し、
 *        直近のリプレイ画像およびパフォーマンスログを出力する外部ライブラリクラス
 */
class PerformanceReporter {
public:
    // マジックナンバー排除のための定数
    static inline const float kDefaultFpsDropThreshold = 30.0f;  // FPS低下検知しきい値
    static inline const int kDefaultCaptureSeconds = 3;           // リプレイ記録時間（秒）
    static inline const int kFpsLogLimit = 300;                   // パフォーマンスログの最大保持件数

public:
    PerformanceReporter() = delete;
    ~PerformanceReporter() = delete;

    /**
     * @brief レポーターの初期化
     * @param device DirectX12デバイス
     * @param commandQueue コマンドキュー（リソースコピーコマンドの発行用）
     * @param width バックバッファの幅
     * @param height バックバッファの高さ
     */
    static void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT width, UINT height);

    /**
     * @brief レポーターのシャットダウンとリソース解放
     */
    static void Finalize();

    /**
     * @brief 毎フレームの更新処理（FPS監視とロード時間監視）
     * @param deltaTime 前フレームからの経過時間（秒）
     * @param currentFps 現在のフレームレート
     * @param memoryUsageMB 現在のメモリ使用量 (MB)
     */
    static void Update(float deltaTime, float currentFps, float memoryUsageMB);

    /**
     * @brief ロード開始の通知（ロード時間計測用）
     */
    static void StartLoadTimer();

    /**
     * @brief ロード終了の通知（ロード時間計測用、基準値を超えたら自動ダンプ）
     * @param loadName ロードしたシーンやエリアの名前
     * @param maxAllowedSeconds 許容される最大ロード時間（秒、これを超えたら警告）
     */
    static void EndLoadTimer(const std::string& loadName, float maxAllowedSeconds = 2.0f);

    /**
     * @brief 現在のバックバッファをキャプチャし、リングバッファに保存する
     * @param backBuffer レンダリング完了後のバックバッファリソース
     * @param currentState バックバッファの現在のリソース状態（通常は D3D12_RESOURCE_STATE_PRESENT）
     */
    static void CaptureFrame(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer, D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_PRESENT);

    /**
     * @brief 手動または自動でパフォーマンスレポートのダンプを実行する
     * @param reason ダンプが発生した理由（例: "FPS_DROP", "LONG_LOAD", "MANUAL_TRIGGER"）
     * @param detail 詳細情報文字列
     */
    static void TriggerReport(const std::string& reason, const std::string& detail = "");

    /**
     * @brief 監視設定 of 変更
     */
    static void SetFpsDropThreshold(float threshold) { fpsDropThreshold_ = threshold; }
    static void SetMonitoringEnabled(bool enabled) { isEnabled_ = enabled; }

private:
    // キャプチャされた1フレームの情報
    struct CapturedFrame {
        Microsoft::WRL::ComPtr<ID3D12Resource> gpuTexture; // GPU上のテクスチャ
        float timestamp = 0.0f;
    };

    // パフォーマンスログのエントリ
    struct PerfLogEntry {
        float time = 0.0f;
        float fps = 0.0f;
        float memory = 0.0f;
    };

    // 内部ユーティリティ
    static void SaveMp4File(const std::wstring& filePath, const std::vector<CapturedFrame>& textures, UINT width, UINT height);
    static void SavePngFile(const std::wstring& filePath, BYTE* rawRgbaData, UINT width, UINT height);
    static void DumpReportPackage(const std::string& reason, const std::string& detail);
    static std::wstring ConvertToWstring(const std::string& str);

private:
    static ID3D12Device* device_;
    static ID3D12CommandQueue* commandQueue_;
    static UINT bufferWidth_;
    static UINT bufferHeight_;

    static float fpsDropThreshold_;
    static bool isEnabled_;
    static bool isTriggeredThisFrame_;
    static float cooldownTimer_;

    // ロード時間計測用
    static std::chrono::steady_clock::time_point loadStartTime_;
    static bool isLoading_;

    // リプレイ用リングバッファ
    static std::deque<CapturedFrame> frameRingBuffer_;
    static size_t maxRingBufferSize_;
    static float runningTime_;

    // 統計ログバッファ
    static std::deque<PerfLogEntry> perfLog_;
};
