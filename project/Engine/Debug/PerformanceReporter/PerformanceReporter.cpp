#include "PerformanceReporter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <format>
#include <algorithm>
#include <iomanip>
#include <shellapi.h> // シェル起動用
#include "Engine/Base/Log/Log.h" // ゲーム内ログ出力用

// Media Foundation 関連の初期化
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

// WIC (Windows Imaging Component) 関連の初期化
#include <wincodec.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Windowscodecs.lib")


// 静的メンバ変数の実体化
ID3D12Device* PerformanceReporter::device_ = nullptr;
ID3D12CommandQueue* PerformanceReporter::commandQueue_ = nullptr;
UINT PerformanceReporter::bufferWidth_ = 0;
UINT PerformanceReporter::bufferHeight_ = 0;

float PerformanceReporter::fpsDropThreshold_ = PerformanceReporter::kDefaultFpsDropThreshold;
bool PerformanceReporter::isEnabled_ = true;
bool PerformanceReporter::isTriggeredThisFrame_ = false;
float PerformanceReporter::cooldownTimer_ = 0.0f;

std::chrono::steady_clock::time_point PerformanceReporter::loadStartTime_;
bool PerformanceReporter::isLoading_ = false;

std::deque<PerformanceReporter::CapturedFrame> PerformanceReporter::frameRingBuffer_;
size_t PerformanceReporter::maxRingBufferSize_ = 30; // 10fps で 3秒分 (30フレーム)
float PerformanceReporter::runningTime_ = 0.0f;

std::deque<PerformanceReporter::PerfLogEntry> PerformanceReporter::perfLog_;

namespace {
    // 10fps 間隔でキャプチャするためのタイマー (100msに1回)
    constexpr float kCaptureInterval = 1.0f / 10.0f;
    float s_captureTimer = 0.0f;

    // クールタイム（連続ダンプ防止：5秒）
    constexpr float kCooldownDuration = 5.0f;

    // サーキュラーバッファの書き込みインデックス
    size_t s_writeIndex = 0;

    // 遅延ダンプ用フラグと一時変数
    bool s_shouldDumpNextFrame = false;
    std::string s_triggerReason = "";
    std::string s_triggerDetail = "";
}

void PerformanceReporter::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT width, UINT height) {
    device_ = device;
    commandQueue_ = commandQueue;
    bufferWidth_ = width;
    bufferHeight_ = height;
    isEnabled_ = true;
    isTriggeredThisFrame_ = false;
    cooldownTimer_ = 0.0f;
    runningTime_ = 0.0f;
    s_captureTimer = 0.0f;
    s_writeIndex = 0;
    s_shouldDumpNextFrame = false;
    s_triggerReason = "";
    s_triggerDetail = "";

    frameRingBuffer_.clear();
    perfLog_.clear();
}

void PerformanceReporter::Finalize() {
    frameRingBuffer_.clear();
    perfLog_.clear();
    device_ = nullptr;
    commandQueue_ = nullptr;
}

void PerformanceReporter::Update(float deltaTime, float currentFps, float memoryUsageMB) {
    if (!isEnabled_ || !device_) return;

    runningTime_ += deltaTime;
    s_captureTimer += deltaTime;

    if (cooldownTimer_ > 0.0f) {
        cooldownTimer_ -= deltaTime;
    }

    // 統計ログの記録（毎フレームではなく、ある程度の間隔で記録しても良いが、ここでは単純に毎フレーム追加）
    PerfLogEntry entry;
    entry.time = runningTime_;
    entry.fps = currentFps;
    entry.memory = memoryUsageMB;
    perfLog_.push_back(entry);

    if (perfLog_.size() > kFpsLogLimit) {
        perfLog_.pop_front();
    }

    // 自動トリガー判定：FPSが閾値を下回った場合
    if (cooldownTimer_ <= 0.0f && currentFps > 0.0f && currentFps < fpsDropThreshold_) {
        std::string detail = std::format("FPS dropped to {:.2f} (Threshold: {:.2f} FPS)", currentFps, fpsDropThreshold_);
        TriggerReport("FPS_DROP", detail);
    }
}

void PerformanceReporter::StartLoadTimer() {
    loadStartTime_ = std::chrono::steady_clock::now();
    isLoading_ = true;
}

void PerformanceReporter::EndLoadTimer(const std::string& loadName, float maxAllowedSeconds) {
    if (!isLoading_) return;
    isLoading_ = false;

    auto endTime = std::chrono::steady_clock::now();
    float loadDuration = std::chrono::duration<float>(endTime - loadStartTime_).count();

    // ロード時間が許容値を超えた場合に自動トリガー
    if (loadDuration > maxAllowedSeconds) {
        std::string detail = std::format("Scene/Area '{}' load took {:.2f} seconds (Max Allowed: {:.2f}s)", loadName, loadDuration, maxAllowedSeconds);
        TriggerReport("LONG_LOAD", detail);
    }
}

void PerformanceReporter::CaptureFrame(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer, D3D12_RESOURCE_STATES currentState) {
    if (!isEnabled_ || !device_ || !backBuffer || !cmdList) return;

    D3D12_RESOURCE_DESC srcDesc = backBuffer->GetDesc();

    // 初回実行時、またはコピー元のサイズ・フォーマットが動的に変わった場合、リングバッファ用テクスチャを全再生成
    bool needsRecreate = frameRingBuffer_.empty();
    if (!frameRingBuffer_.empty()) {
        D3D12_RESOURCE_DESC destDesc = frameRingBuffer_[0].gpuTexture->GetDesc();
        if (destDesc.Width != srcDesc.Width || destDesc.Height != srcDesc.Height || destDesc.Format != srcDesc.Format) {
            needsRecreate = true;
        }
    }

    if (needsRecreate) {
        frameRingBuffer_.clear();
        bufferWidth_ = static_cast<UINT>(srcDesc.Width);
        bufferHeight_ = srcDesc.Height;

        for (size_t i = 0; i < maxRingBufferSize_; ++i) {
            CapturedFrame cf;
            cf.timestamp = 0.0f;

            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width = bufferWidth_;
            desc.Height = bufferHeight_;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = srcDesc.Format; // コピー元と100%同一のフォーマット
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            HRESULT hr = device_->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&cf.gpuTexture)
            );
            
            if (SUCCEEDED(hr)) {
                frameRingBuffer_.push_back(cf);
            }
        }
        s_writeIndex = 0;
    }

    if (frameRingBuffer_.empty()) return;

    // キャプチャ間隔（15fps）を制御
    if (s_captureTimer >= kCaptureInterval) {
        s_captureTimer = 0.0f;

        // 対象のキャプチャ先リソース
        auto& targetFrame = frameRingBuffer_[s_writeIndex];
        if (targetFrame.gpuTexture) {
            // バリアを張って COPY_SOURCE / COPY_DEST に遷移
            D3D12_RESOURCE_BARRIER barriers[2]{};
            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Transition.pResource = backBuffer;
            barriers[0].Transition.StateBefore = currentState;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Transition.pResource = targetFrame.gpuTexture.Get();
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            cmdList->ResourceBarrier(2, barriers);

            // VRAM間のコピー実行 (超高速、CPU同期待機なし)
            cmdList->CopyResource(targetFrame.gpuTexture.Get(), backBuffer);

            // バリアを戻す
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers[0].Transition.StateAfter = currentState;

            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;

            cmdList->ResourceBarrier(2, barriers);

            // タイムスタンプ記録とインデックス進行
            targetFrame.timestamp = runningTime_;
            s_writeIndex = (s_writeIndex + 1) % frameRingBuffer_.size();
        }
    }

    // 遅延ダンプフラグが立っていたら、ここでダンプを実行する
    if (s_shouldDumpNextFrame) {
        s_shouldDumpNextFrame = false;
        DumpReportPackage(s_triggerReason, s_triggerDetail);
    }
}

void PerformanceReporter::TriggerReport(const std::string& reason, const std::string& detail) {
    if (cooldownTimer_ > 0.0f) return;
    cooldownTimer_ = kCooldownDuration;

    // ゲーム内コンソールへ進捗状況を出力
    Log::Write("[システム] パフォーマンスレポートとMP4動画(直前3秒間)を出力しています。画面が一瞬静止しますが少々お待ちください...");

    // フラグを立てて、描画フレームの最後でのダンプ処理実行をスケジュールする
    s_shouldDumpNextFrame = true;
    s_triggerReason = reason;
    s_triggerDetail = detail;
}

void PerformanceReporter::SavePngFile(const std::wstring& filePath, BYTE* rawRgbaData, UINT width, UINT height) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return;

    Microsoft::WRL::ComPtr<IWICStream> stream;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr)) return;

    hr = stream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) return;

    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
    hr = factory->CreateEncoder(GUID_ContainerFormatPng, NULL, &encoder);
    if (FAILED(hr)) return;

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) return;

    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frameEncode;
    hr = encoder->CreateNewFrame(&frameEncode, NULL);
    if (FAILED(hr)) return;

    hr = frameEncode->Initialize(NULL);
    if (FAILED(hr)) return;

    hr = frameEncode->SetSize(width, height);
    if (FAILED(hr)) return;

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppRGBA;
    hr = frameEncode->SetPixelFormat(&format);
    if (FAILED(hr)) return;

    // RGBA データの書き込み (DX12の R8G8B8A8 形式)
    UINT stride = width * 4;
    UINT bufferSize = stride * height;
    hr = frameEncode->WritePixels(height, stride, bufferSize, rawRgbaData);
    if (FAILED(hr)) return;

    hr = frameEncode->Commit();
    if (FAILED(hr)) return;

    hr = encoder->Commit();
    if (FAILED(hr)) return;
}

void PerformanceReporter::DumpReportPackage(const std::string& reason, const std::string& detail) {
    // 1. ディレクトリ生成
    std::time_t t = std::time(nullptr);
    std::tm tm_info;
    localtime_s(&tm_info, &t);
    char folderBuffer[64];
    std::strftime(folderBuffer, sizeof(folderBuffer), "%Y%m%d_%H%M%S", &tm_info);
    std::string folderName = "out/performance_reports/report_" + std::string(folderBuffer);
    
    std::filesystem::create_directories(folderName);

    // 古い順に並べ替えたテクスチャ配列を作成
    size_t ringSize = frameRingBuffer_.size();
    std::vector<CapturedFrame> orderedFrames;
    orderedFrames.reserve(ringSize);
    for (size_t i = 0; i < ringSize; ++i) {
        size_t idx = (s_writeIndex + i) % ringSize;
        if (frameRingBuffer_[idx].gpuTexture) {
            orderedFrames.push_back(frameRingBuffer_[idx]);
        }
    }

    // 2. MP4ビデオのエンコード出力 (直前3秒間)
    std::string mp4Path = folderName + "/replay.mp4";
    SaveMp4File(ConvertToWstring(mp4Path), orderedFrames, bufferWidth_, bufferHeight_);

    // 3. 最新の1フレームを screenshot.bmp として保存
    if (!orderedFrames.empty()) {
        auto& latestFrame = orderedFrames.back();
        
        UINT rowPitch = (bufferWidth_ * 4 + 255) & ~255;
        UINT64 bufferSize = rowPitch * bufferHeight_;
        std::vector<BYTE> rgbaBuffer(bufferWidth_ * bufferHeight_ * 4);

        Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer;
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC bufferDesc{};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = bufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        device_->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&readbackBuffer)
        );

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
        device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = latestFrame.gpuTexture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = latestFrame.gpuTexture.Get();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dstLocation{};
        dstLocation.pResource = readbackBuffer.Get();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLocation.PlacedFootprint.Offset = 0;
        dstLocation.PlacedFootprint.Footprint.Width = bufferWidth_;
        dstLocation.PlacedFootprint.Footprint.Height = bufferHeight_;
        dstLocation.PlacedFootprint.Footprint.Depth = 1;
        dstLocation.PlacedFootprint.Footprint.Format = latestFrame.gpuTexture->GetDesc().Format;
        dstLocation.PlacedFootprint.Footprint.RowPitch = rowPitch;

        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->Close();
        ID3D12CommandList* lists[] = { cmdList.Get() };
        commandQueue_->ExecuteCommandLists(1, lists);

        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        commandQueue_->Signal(fence.Get(), 1);
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        if (fence->GetCompletedValue() < 1) {
            fence->SetEventOnCompletion(1, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
        }
        CloseHandle(eventHandle);

        void* mappedData = nullptr;
        D3D12_RANGE readRange{ 0, rowPitch * bufferHeight_ };
        if (SUCCEEDED(readbackBuffer->Map(0, &readRange, &mappedData))) {
            BYTE* srcBytes = reinterpret_cast<BYTE*>(mappedData);
            for (UINT y = 0; y < bufferHeight_; ++y) {
                std::memcpy(
                    rgbaBuffer.data() + (y * bufferWidth_ * 4),
                    srcBytes + (y * rowPitch),
                    bufferWidth_ * 4
                );
            }
            readbackBuffer->Unmap(0, nullptr);

            std::string pngPath = folderName + "/screenshot.png";
            SavePngFile(ConvertToWstring(pngPath), rgbaBuffer.data(), bufferWidth_, bufferHeight_);
        }
    }

    // 4. system_log.json の出力
    std::string jsonPath = folderName + "/system_log.json";
    std::ofstream jsonOfs(jsonPath);
    if (jsonOfs.is_open()) {
        jsonOfs << "{\n";
        jsonOfs << "  \"reason\": \"" << reason << "\",\n";
        jsonOfs << "  \"detail\": \"" << detail << "\",\n";
        char timeBuffer[64];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm_info);
        jsonOfs << "  \"time_triggered\": \"" << timeBuffer << "\",\n";
        jsonOfs << "  \"resolution\": \"" << bufferWidth_ << "x" << bufferHeight_ << "\",\n";
        jsonOfs << "  \"logs\": [\n";

        for (size_t i = 0; i < perfLog_.size(); ++i) {
            jsonOfs << "    {\n";
            jsonOfs << "      \"time\": " << std::fixed << std::setprecision(3) << perfLog_[i].time << ",\n";
            jsonOfs << "      \"fps\": " << std::fixed << std::setprecision(2) << perfLog_[i].fps << ",\n";
            jsonOfs << "      \"memory_mb\": " << std::fixed << std::setprecision(2) << perfLog_[i].memory << "\n";
            jsonOfs << "    }" << (i == perfLog_.size() - 1 ? "" : ",") << "\n";
        }

        jsonOfs << "  ]\n";
        jsonOfs << "}\n";
    }

    // 4. prompt.md (LLM 解析依頼用プロンプト) の出力
    std::string promptPath = folderName + "/prompt.md";
    std::ofstream promptOfs(promptPath);
    if (promptOfs.is_open()) {
        promptOfs << "あなたはC++およびDirectX12ゲームエンジンのパフォーマンス最適化の超一流エキスパートエンジニアです。\n";
        promptOfs << "以下の実行時パフォーマンスデータおよび添付されたリプレイ画像をもとに、FPS低下が発生した原因を推測し、考えられるボトルネックの特定と具体的なコードレベルでの修正案（最適化案）を日本語で提示してください。\n\n";
        
        promptOfs << "# 【AI解析依頼】パフォーマンス低下スパイクの調査\n\n";
        promptOfs << "## 1. 発生時の詳細コンテキスト\n";
        promptOfs << "- **検知トリガー理由**: `" << reason << "`\n";
        promptOfs << "- **詳細情報**: " << detail << "\n";
        char timeBuffer[64];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm_info);
        promptOfs << "- **発生日時**: " << timeBuffer << "\n";
        promptOfs << "- **画面解像度**: " << bufferWidth_ << "x" << bufferHeight_ << "\n\n";

        promptOfs << "## 2. パフォーマンスログデータ (時系列)\n";
        promptOfs << "| 経過時間 (秒) | FPS | メモリ使用量 (MB) |\n";
        promptOfs << "| :--- | :--- | :--- |\n";
        
        // ログデータを最大30行程度サンプリングしてマークダウンテーブル化
        size_t step = std::max<size_t>(1, perfLog_.size() / 30);
        for (size_t i = 0; i < perfLog_.size(); i += step) {
            promptOfs << "| " << std::fixed << std::setprecision(2) << perfLog_[i].time << "s | "
                      << std::fixed << std::setprecision(1) << perfLog_[i].fps << " | "
                      << std::fixed << std::setprecision(1) << perfLog_[i].memory << " MB |\n";
        }
        promptOfs << "\n";

        promptOfs << "## 3. 同梱のリプレイデータについて\n";
        promptOfs << "- スパイク発生の直前3秒間を記録したMP4動画ファイル（`replay.mp4`）および静止画（`screenshot.bmp`）がこのフォルダに保存されています。\n";
        promptOfs << "- ビューワー上で動画として再生・シーク可能です。\n\n";

        promptOfs << "## 4. あなた（LLM）への調査指示事項\n";
        promptOfs << "以下の仮説とポイントを重点的に検証してください：\n";
        promptOfs << "1. **CPU/GPUボトルネックの判定**: 時系列ログでFPSが落ち込んでいる瞬間、メモリ使用量に急激なスパイクやリーク（増加し続けている状態）は見られますか？\n";
        promptOfs << "2. **描画負荷との連動**: 添付された画像・動画において、FPSが低下しているフレームに「大量のオブジェクト」「パーティクルの密集」「特定のUI」などが描画されていませんか？\n";
        promptOfs << "3. **コード側の懸念箇所**: C++ゲームエンジンでカクつきが発生する一般的な要因（例：動的なメモリ確保(`new`/`vector::push_back`など)の頻発、DirectX12のバリア遷移の多発、CPU側の同期ファイルI/Oによるブロッキングなど）と照らし合わせ、どの処理を最適化すべきか考察してください。\n";
    }

    std::cout << "[PerformanceReporter] Report output success to: " << folderName << std::endl;

    // ビューワーを自動非同期起動
    ShellExecuteA(NULL, "open", "PerformanceViewer.exe", NULL, NULL, SW_SHOW);
}

std::wstring PerformanceReporter::ConvertToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], sizeNeeded);
    return wstrTo;
}

void PerformanceReporter::SaveMp4File(const std::wstring& filePath, const std::vector<CapturedFrame>& textures, UINT width, UINT height) {
    if (textures.empty() || !device_ || !commandQueue_) return;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return;

    Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter;
    hr = MFCreateSinkWriterFromURL(filePath.c_str(), NULL, NULL, &sinkWriter);
    if (FAILED(hr)) {
        MFShutdown();
        return;
    }

    // ビデオ出力ストリームの設定 (H.264 MP4)
    Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeOut;
    hr = MFCreateMediaType(&mediaTypeOut);
    mediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    mediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, 2000000); // 2Mbps
    MFSetAttributeSize(mediaTypeOut.Get(), MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(mediaTypeOut.Get(), MF_MT_FRAME_RATE, 15, 1); // 15fps
    MFSetAttributeRatio(mediaTypeOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    mediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    DWORD streamIndex;
    hr = sinkWriter->AddStream(mediaTypeOut.Get(), &streamIndex);
    if (FAILED(hr)) {
        MFShutdown();
        return;
    }

    // ビデオ入力ストリームの設定 (RGB32)
    Microsoft::WRL::ComPtr<IMFMediaType> mediaTypeIn;
    hr = MFCreateMediaType(&mediaTypeIn);
    mediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    MFSetAttributeSize(mediaTypeIn.Get(), MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(mediaTypeIn.Get(), MF_MT_FRAME_RATE, 15, 1);
    MFSetAttributeRatio(mediaTypeIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    mediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    hr = sinkWriter->SetInputMediaType(streamIndex, mediaTypeIn.Get(), NULL);
    if (FAILED(hr)) {
        MFShutdown();
        return;
    }

    hr = sinkWriter->BeginWriting();
    if (FAILED(hr)) {
        MFShutdown();
        return;
    }

    // リードバック用バッファを作成
    UINT rowPitch = (width * 4 + 255) & ~255;
    UINT64 bufferSize = rowPitch * height;
    std::vector<BYTE> rgbaBuffer(width * height * 4);

    Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer;
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;
    
    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&readbackBuffer)
    );
    if (FAILED(hr)) {
        MFShutdown();
        return;
    }

    LONGLONG rtStart = 0;
    LONGLONG frameDuration = 10 * 1000 * 1000 / 15; // 15fpsでの1フレームの時間（100ナノ秒単位）

    for (size_t i = 0; i < textures.size(); ++i) {
        auto& frame = textures[i];
        if (!frame.gpuTexture) continue;

        // VRAMからリードバックへコピー
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
        
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
        device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = frame.gpuTexture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = frame.gpuTexture.Get();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dstLocation{};
        dstLocation.pResource = readbackBuffer.Get();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLocation.PlacedFootprint.Offset = 0;
        dstLocation.PlacedFootprint.Footprint.Width = width;
        dstLocation.PlacedFootprint.Footprint.Height = height;
        dstLocation.PlacedFootprint.Footprint.Depth = 1;
        dstLocation.PlacedFootprint.Footprint.Format = frame.gpuTexture->GetDesc().Format;
        dstLocation.PlacedFootprint.Footprint.RowPitch = rowPitch;

        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->Close();

        ID3D12CommandList* lists[] = { cmdList.Get() };
        commandQueue_->ExecuteCommandLists(1, lists);

        // 同期待機
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        commandQueue_->Signal(fence.Get(), 1);
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        if (fence->GetCompletedValue() < 1) {
            fence->SetEventOnCompletion(1, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
        }
        CloseHandle(eventHandle);

        // メモリMap & 順序変換してバッファへコピー
        void* mappedData = nullptr;
        D3D12_RANGE readRange{ 0, rowPitch * height };
        if (SUCCEEDED(readbackBuffer->Map(0, &readRange, &mappedData))) {
            BYTE* srcBytes = reinterpret_cast<BYTE*>(mappedData);
            for (UINT y = 0; y < height; ++y) {
                // Media Foundation RGB32 は上下反転（下から上）
                UINT srcY = height - 1 - y;
                BYTE* srcRow = srcBytes + (srcY * rowPitch);
                BYTE* dstRow = rgbaBuffer.data() + (y * width * 4);
                
                for (UINT x = 0; x < width; ++x) {
                    UINT srcIdx = x * 4;
                    UINT dstIdx = x * 4;
                    // DX12 RGBA ➔ BGRA (Media Foundation RGB32 用)
                    dstRow[dstIdx + 0] = srcRow[srcIdx + 2]; // B
                    dstRow[dstIdx + 1] = srcRow[srcIdx + 1]; // G
                    dstRow[dstIdx + 2] = srcRow[srcIdx + 0]; // R
                    dstRow[dstIdx + 3] = srcRow[srcIdx + 3]; // A
                }
            }
            readbackBuffer->Unmap(0, nullptr);

            // メディアサンプルの書き込み
            Microsoft::WRL::ComPtr<IMFSample> sample;
            hr = MFCreateSample(&sample);
            if (SUCCEEDED(hr)) {
                Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
                hr = MFCreateMemoryBuffer(width * height * 4, &buffer);
                if (SUCCEEDED(hr)) {
                    BYTE* dataDest = nullptr;
                    hr = buffer->Lock(&dataDest, NULL, NULL);
                    if (SUCCEEDED(hr)) {
                        std::memcpy(dataDest, rgbaBuffer.data(), width * height * 4);
                        buffer->Unlock();
                        buffer->SetCurrentLength(width * height * 4);
                        sample->AddBuffer(buffer.Get());
                        sample->SetSampleTime(rtStart);
                        sample->SetSampleDuration(frameDuration);
                        sinkWriter->WriteSample(streamIndex, sample.Get());
                    }
                }
            }
            rtStart += frameDuration;
        }
    }

    sinkWriter->Finalize();
    MFShutdown();
}
