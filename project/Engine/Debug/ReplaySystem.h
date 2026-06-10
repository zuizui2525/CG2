#ifdef _USEIMGUI
#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>

class ReplaySystem {
public:
    static ReplaySystem* GetInstance();

    void Initialize(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, int32_t width, int32_t height);
    void Finalize();
    void ClearGarbage();


    // 毎フレームの記録 (ゲーム更新ループ内で呼ぶ)
    void RecordFrame(ID3D12GraphicsCommandList* commandList, ID3D12Resource* sourceTexture, D3D12_GPU_DESCRIPTOR_HANDLE sourceSrv, float fps, float memory);


    // 一時停止状態の管理
    void SetPause(bool pause);
    bool IsPaused() const { return isPaused_; }

    // リプレイ表示用のシーク制御 (0.0f = 最も古い過去, 1.0f = 一時停止した瞬間)
    void SetSeekPos(float progress);
    float GetSeekPos() const { return seekProgress_; }
    bool IsReplayPlaying() const { return isReplayPlaying_; }
    void SetReplayPlaying(bool play) { isReplayPlaying_ = play; }
    void UpdateReplayPlay(float deltaTime);

    // リサイズ時のテクスチャプール再構成
    void OnResize(int32_t width, int32_t height);

    // 現在シークされているフレームの情報を取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetReplaySrvGpuHandle() const;
    void SeekToTimestamp(float timestamp);
    int32_t GetEffectiveRecordCount(int32_t* outStartIdx = nullptr) const;
    float GetLatestRecordTimestamp() const;
    float GetReplayFps(int32_t targetIdx) const;
    float GetReplayMemory(int32_t targetIdx) const;
    float GetReplayTimeOffset(int32_t targetIdx) const; // 現在から何秒前か

    // リプレイ中のコンソール表示用タイムスタンプ上限を取得 (一時停止解除時は -1.0f)
    float GetReplayMaxTimestamp() const;

    // パフォーマンスモニター用の履歴取得
    void GetReplayHistory(int32_t targetIdx, float* outFpsHistory, float* outMemHistory, int32_t historySize);

    // リプレイ表示更新処理 (毎フレームのImGui描画等のタイミングでcommandListを渡して呼ぶ)
    void UpdateReplayDisplay(ID3D12GraphicsCommandList* commandList);

    int32_t GetRecordCount() const { return static_cast<int32_t>(records_.size()); }

private:
    ReplaySystem() = default;
    ~ReplaySystem() = default;

    struct FrameRecord {
        Microsoft::WRL::ComPtr<ID3D12Resource> texture; // 画面キャプチャリソース
        float fps = 0.0f;
        float memory = 0.0f;
        float timestamp = 0.0f; // 起動からの経過時間（秒）
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle{}; // 個別SRV CPUハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle{}; // 個別SRV GPUハンドル
    };

    void RecreatePool();

    static ReplaySystem* instance_;

    ID3D12Device* device_ = nullptr;
    ID3D12DescriptorHeap* srvHeap_ = nullptr;
    int32_t currentWidth_ = 0;
    int32_t currentHeight_ = 0;

    bool isPaused_ = false;
    bool isReplayPlaying_ = false; // リプレイの自動送り再生中フラグ
    bool needsCopy_ = false; // 表示更新が必要になったフラグ
    float seekProgress_ = 1.0f; // 0.0f 〜 1.0f

    // 記録バッファ (60fpsで5秒分 = 300フレーム)
    std::vector<FrameRecord> records_;
    int32_t writeIndex_ = 0;
    
    // マジックナンバー排除用の定数
    static constexpr int32_t kMaxFrames = 1800; // 記録する最大フレーム数 (30fps * 60秒 = 1800)
    static constexpr int32_t kRecordInterval = 2; // 2フレームに1回記録 (30fps)
    static constexpr UINT kReservedSrvIndexReplayStart = 200; // Replay用SRVの開始スロットインデックス
    static constexpr int32_t kShrinkWidth = 640;  // 縮小録画時の幅
    static constexpr int32_t kShrinkHeight = 360; // 縮小録画時の高さ


    mutable D3D12_GPU_DESCRIPTOR_HANDLE lastPausedGpuHandle_{}; // ポーズ解除した瞬間の静止表示用SRVハンドル


    int32_t frameCounter_ = 0;
    int32_t totalCreatedCount_ = 0; // 生成された履歴テクスチャの累計数（SRVインデックス配信用）
    
    // リサイズ時に一時的に古いテクスチャの寿命を延ばすための遅延解放バッファ
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> garbageCollectTextures_;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> shrinkRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> shrinkPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeapShrink_; // 縮小描画用の一時RTVヒープ (サイズ1)


};
#endif
