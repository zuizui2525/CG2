#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/StringUtility.h"
#include "imgui.h"
#include <intrin.h>

#ifdef _USEIMGUI
#include "Engine/Debug/ReplaySystem.h"
#endif

// 静的メンバ変数の実体定義
std::ofstream Log::logStream_;
std::string Log::logFileName_;
bool Log::isInitialized_ = false;
std::chrono::steady_clock::time_point Log::startTime_;
float Log::activeElapsedTime_ = 0.0f;

std::vector<std::string> Log::logMessages_;
std::vector<float> Log::logTimestamps_;
bool Log::showConsole_ = true;
size_t Log::lastLogSize_ = 0;

void Log::Initialize() {
    if (isInitialized_) return;
    isInitialized_ = true;
    startTime_ = std::chrono::steady_clock::now();

    // ディレクトリ作成
    std::filesystem::create_directory("logs");

    // 現在時刻を取得（秒単位に丸める）
    auto now = std::chrono::system_clock::now();
    auto nowSec = std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSec };

    // ファイル名生成
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
    logFileName_ = "logs/" + dateString + ".log";

    // ファイルオープン
    logStream_.open(logFileName_, std::ios::out);
    if (!logStream_) {
        OutputDebugStringW(L"Log file could not be created.\n");
    } else {
        std::wstring wLogFile = ConvertString(logFileName_);
        OutputDebugStringW((L"Log file created: " + wLogFile + L"\n").c_str());
    }
}

void Log::Write(const std::string& message) {
    if (!isInitialized_) {
        Initialize();
    }


    // 入力メッセージから末尾の改行文字を取り除く
    std::string cleanMessage = message;
    while (!cleanMessage.empty() && (cleanMessage.back() == '\n' || cleanMessage.back() == '\r')) {
        cleanMessage.pop_back();
    }

    // メッセージが空の場合は、タイムスタンプを付与せず改行のみを出力して早期リターン
    if (cleanMessage.empty()) {
        if (logStream_.is_open()) {
            logStream_ << std::endl;
        }
        OutputDebugStringW(L"\n");
        return;
    }

    // 起動からの実経過時間を算出
    float realElapsedTime = 0.0f;
    if (isInitialized_) {
        auto now = std::chrono::steady_clock::now();
        realElapsedTime = std::chrono::duration<float>(now - startTime_).count();
    }

    float elapsedSeconds = activeElapsedTime_;
    long long totalMs = static_cast<long long>(elapsedSeconds * 1000.0f);

    // ミリ秒、秒、分、時間を計算
    long long ms = totalMs % 1000;
    long long totalSec = totalMs / 1000;
    long long s = totalSec % 60;
    long long totalMin = totalSec / 60;
    long long m = totalMin % 60;
    long long h = totalMin / 60;

    // 1分経過（分の繰り上がり）時の自動空白および経過時間マーカー挿入（文字詰まり・視認性対策）
    static long long lastMin = 0;
    if (totalMin > lastMin) {
        lastMin = totalMin;
        
        std::string markerStr = std::format("\n[real|{:7.3f}s] [game|{:02d}h:{:02d}m:00.000s] -------------------- 【 {}分経過 】 --------------------\n", realElapsedTime, h, m, totalMin);
        std::wstring wMarkerStr = ConvertString(markerStr);

        if (logStream_.is_open()) {
            logStream_ << markerStr << std::endl;
        }
        OutputDebugStringW(wMarkerStr.c_str());
    }

    // 指定フォーマットで時間文字列を作成
    std::string timeStr = std::format("[real|{:7.3f}s] [game|{:02d}h:{:02d}m:{:02d}.{:03d}s] ", realElapsedTime, h, m, s, ms);
    std::string fullMessage = timeStr + cleanMessage;

    // バッファへの蓄積（リングバッファ処理）
    if (logMessages_.size() >= kMaxLogLines) {
        logMessages_.erase(logMessages_.begin());
        if (!logTimestamps_.empty()) {
            logTimestamps_.erase(logTimestamps_.begin());
        }
    }
    logMessages_.push_back(fullMessage);
    logTimestamps_.push_back(elapsedSeconds);

    if (logStream_.is_open()) {
        logStream_ << fullMessage << std::endl;
    }

    // デバッグ出力（OutputDebugStringWを使用して文字化けを完全に解消！）
    std::wstring wMsg = ConvertString(fullMessage);
    OutputDebugStringW((wMsg + L"\n").c_str());
}

void Log::Write(std::ostream& os, const std::string& message) {
    // コンソール等への出力（元メッセージの改行を維持して出力）
    os << message;
    if (message.empty() || message.back() != '\n') {
        os << std::endl;
    }
    Write(message); // ファイル＆デバッグ出力にも流す
}

void Log::Write(const std::wstring& message) {
    Write(ConvertString(message));
}

void Log::Write(std::ostream& os, const std::wstring& message) {
    Write(os, ConvertString(message));
}

const std::vector<std::string>& Log::GetLogMessages() {
    return logMessages_;
}

void Log::ClearLog() {
    logMessages_.clear();
    logTimestamps_.clear();
}

void Log::DrawConsoleWindow(float maxTimestamp) {
#ifdef _USEIMGUI
    if (!showConsole_) return;

    if (ImGui::Begin("Console", &showConsole_)) {
        if (ImGui::Button("Clear")) {
            ClearLog();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Max lines: %zu", kMaxLogLines);

        // タイムスタンプ文字列フォーマット用ラムダ式（プレーン版）
        auto FormatPlainGameTimestamp = [](float seconds) -> std::string {
            long long elapsedMs = static_cast<long long>(seconds * 1000.0f);
            long long ms = elapsedMs % 1000;
            long long totalSec = elapsedMs / 1000;
            long long s = totalSec % 60;
            long long m = (totalSec / 60) % 60;
            long long h = totalSec / 3600;
            return std::format("{:02d}h:{:02d}m:{:02d}.{:03d}s", h, m, s, ms);
        };

        static bool filterReplayRangeOnly = false;
        static bool lastPaused = false;
        bool isPaused = ReplaySystem::GetInstance()->IsPaused();

        // 最新のリプレイレコードのタイムスタンプを取得
        float latestReplayTime = ReplaySystem::GetInstance()->GetLatestRecordTimestamp();
        constexpr float kMaxReplayTimeRange = 60.0f;

        // 状態遷移の検知による自動ON/OFF制御
        if (isPaused != lastPaused) {
            if (isPaused) {
                filterReplayRangeOnly = true; // ポーズ時は自動ON
            } else {
                filterReplayRangeOnly = false; // 実行再開時は自動OFF
            }
            lastPaused = isPaused;
        }

        // チェックボックスの常時表示と無効化UIの制御
        ImGui::SameLine();
        if (!isPaused) {
            ImGui::BeginDisabled();
            ImGui::Checkbox("Replay Range Only", &filterReplayRangeOnly);
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[Game Running: N/A]");
        } else {
            ImGui::Checkbox("Replay Range Only", &filterReplayRangeOnly);
        }

        // リプレイ時間範囲（開始・終了時間）のテキスト表示
        if (filterReplayRangeOnly && latestReplayTime >= 0.0f) {
            float earliestReplayTime = latestReplayTime - kMaxReplayTimeRange;
            if (earliestReplayTime < 0.0f) {
                earliestReplayTime = 0.0f;
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 1.0f, 1.0f), 
                "| Range: %s - %s", 
                FormatPlainGameTimestamp(earliestReplayTime).c_str(), 
                FormatPlainGameTimestamp(latestReplayTime).c_str());
        }

        ImGui::Separator();

        // スクロール可能な子ウィンドウ
        ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        // リアルタイム起動経過時間
        float realTime = 0.0f;
        if (isInitialized_) {
            auto now = std::chrono::steady_clock::now();
            realTime = std::chrono::duration<float>(now - startTime_).count();
        }

        // タイムスタンプ文字列フォーマット用ラムダ式
        auto FormatRealTimestamp = [](float seconds) -> std::string {
            return std::format("[real|{:7.3f}s] ", seconds);
        };
        auto FormatGameTimestamp = [&](float seconds) -> std::string {
            return "[game|" + FormatPlainGameTimestamp(seconds) + "] ";
        };

        // 現在時間 (currentTime) を決定 (通常再生時は現在の経過時間を使用)
        float currentTime = maxTimestamp;
        if (currentTime < 0.0f) {
            currentTime = GetElapsedTime();
        }

        // 最も近いログ行の検索
        size_t nearestIdx = static_cast<size_t>(-1);
        float minDiff = 10000.0f;
        if (!logTimestamps_.empty()) {
            for (size_t i = 0; i < logTimestamps_.size(); ++i) {
                float diff = std::abs(currentTime - logTimestamps_[i]);
                if (diff < minDiff) {
                    minDiff = diff;
                    nearestIdx = i;
                }
            }
        }

        // 被り判定の閾値 (0.15秒 ＝ 150ミリ秒)。一時停止中（リプレイ中）は常に直近のログを被り状態とする。
        constexpr float kOverlapThreshold = 0.15f;
        bool isOverlapped = (nearestIdx != static_cast<size_t>(-1) && (isPaused || minDiff <= kOverlapThreshold));

        // 仮想タイマー行の挿入位置決定
        // logTimestamps_[i] <= currentTime < logTimestamps_[i+1] となる i
        size_t insertAfterIdx = static_cast<size_t>(-1);
        if (!logTimestamps_.empty()) {
            if (currentTime < logTimestamps_[0]) {
                insertAfterIdx = static_cast<size_t>(-2); // 最初のログの前に挿入する特殊フラグ
            } else {
                for (size_t i = 0; i < logTimestamps_.size(); ++i) {
                    if (logTimestamps_[i] <= currentTime) {
                        insertAfterIdx = i;
                    } else {
                        break;
                    }
                }
            }
        }

        // 最初のログより前にタイマーを挿入する場合
        if (!isPaused && !isOverlapped && insertAfterIdx == static_cast<size_t>(-2)) {
            std::string timerStr = "--> " + FormatRealTimestamp(realTime) + FormatGameTimestamp(currentTime) + "------ 【進行中】 ------";
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            ImGui::TextUnformatted(timerStr.c_str());
            ImGui::PopStyleColor();
        }

        bool drawnStartBoundary = false;
        bool drawnEndBoundary = false;

        for (size_t i = 0; i < logMessages_.size(); ++i) {
            const auto& log = logMessages_[i];
            
            // リプレイジャンプ（シーク）の可否を判定
            float earliestReplayTime = latestReplayTime - kMaxReplayTimeRange;
            if (earliestReplayTime < 0.0f) {
                earliestReplayTime = 0.0f;
            }
            // リプレイデータが存在し、かつログタイムスタンプが有効な過去のリプレイ範囲内であること
            // (※ゲームを止めた後のログや、一時停止中の新規ログ (time >= latestReplayTime) はシーク不可とする)
            bool canSeek = (latestReplayTime >= 0.0f && 
                            logTimestamps_[i] >= earliestReplayTime && 
                            logTimestamps_[i] <= latestReplayTime);

            // リプレイ開始境界線の挿入
            if (isPaused && !drawnStartBoundary && logTimestamps_[i] >= earliestReplayTime) {
                std::string boundaryStr = "--> ------ 【リプレイ開始地点: " + FormatPlainGameTimestamp(earliestReplayTime) + "】 ------";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 1.0f, 1.0f));
                ImGui::TextUnformatted(boundaryStr.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Double-click to seek Replay to start time\n[左ダブルクリックでリプレイ開始時間にジャンプ]");
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        ReplaySystem::GetInstance()->SeekToTimestamp(earliestReplayTime);
                    }
                }
                ImGui::PopStyleColor();
                drawnStartBoundary = true;
            }

            // リプレイ終了境界線の挿入
            if (isPaused && !drawnEndBoundary && logTimestamps_[i] > latestReplayTime) {
                std::string boundaryStr = "--> ------ 【リプレイ終了地点: " + FormatPlainGameTimestamp(latestReplayTime) + "】 ------";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 1.0f, 1.0f));
                ImGui::TextUnformatted(boundaryStr.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Double-click to seek Replay to end time\n[左ダブルクリックでリプレイ終了時間にジャンプ]");
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        ReplaySystem::GetInstance()->SeekToTimestamp(latestReplayTime);
                    }
                }
                ImGui::PopStyleColor();
                drawnEndBoundary = true;
            }

            if (filterReplayRangeOnly && !canSeek) {
                continue;
            }

            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルト白
            bool isCurrentActive = (isOverlapped && i == nearestIdx);

            bool shouldFade = false;
            if (latestReplayTime >= 0.0f) {
                shouldFade = !canSeek;
            } else {
                shouldFade = (currentTime - logTimestamps_[i] > kMaxReplayTimeRange);
            }

            // 1. ログ内容に応じた基本色の決定
            if (log.find("error") != std::string::npos || log.find("failed") != std::string::npos || 
                log.find("Error") != std::string::npos || log.find("エラー") != std::string::npos || 
                log.find("ERROR") != std::string::npos) {
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // エラー：マイルドレッド
            } else if (log.find("warning") != std::string::npos || log.find("Warning") != std::string::npos || 
                       log.find("警告") != std::string::npos || log.find("WARNING") != std::string::npos || 
                       log.find("★") != std::string::npos) {
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // 警告：マイルドイエロー
            } else {
                color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // 一般・システム：マイルドグリーン
            }

            // 2. 被り強調、または過去ログのフェードアウト処理
            if (isCurrentActive) {
                color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // 被ったログは鮮やかな赤で表示
            } else if (shouldFade) {
                color.w = 0.4f; // リプレイ不可のログは半透明化
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            
            if (isCurrentActive) {
                // 現在被っている行には目立つ矢印マーカーを付加して描画
                std::string markedLog = "--> " + log;
                ImGui::TextUnformatted(markedLog.c_str());

                // 被ったログ位置が前フレームから変化した瞬間に、画面中央に自動スクロール
                static size_t lastActiveIndex = static_cast<size_t>(-1);
                if (nearestIdx != lastActiveIndex) {
                    ImGui::SetScrollHereY(0.5f); // 画面中央にスクロール
                    lastActiveIndex = nearestIdx;
                }
            } else {
                ImGui::TextUnformatted(log.c_str());
            }

            // 各実ログ行が左ダブルクリックされた際、リプレイ映像をそのログの出力時間に自動ジャンプさせる
            if (ImGui::IsItemHovered()) {
                if (!canSeek) {
                    ImGui::SetTooltip("Cannot seek Replay (Out of Replay Range)\n[リプレイ範囲外のため、このログの時間へはジャンプできません]");
                } else {
                    ImGui::SetTooltip("Double-click to seek Replay to this log's time\n[左ダブルクリックでリプレイをこのログの時間にジャンプ]");
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        ReplaySystem::GetInstance()->SeekToTimestamp(logTimestamps_[i]);
                    }
                }
            }

            ImGui::PopStyleColor();

            // ログの行間に仮想タイマーを挿入する場合
            if (!isPaused && !isOverlapped && insertAfterIdx == i) {
                std::string timerStr = "--> " + FormatRealTimestamp(realTime) + FormatGameTimestamp(currentTime) + "------ 【進行中】 ------";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                ImGui::TextUnformatted(timerStr.c_str());
                ImGui::PopStyleColor();

                // 仮想タイマー行の挿入位置が変わった瞬間に、画面中央に自動スクロール
                static size_t lastInsertIdx = static_cast<size_t>(-1);
                if (insertAfterIdx != lastInsertIdx) {
                    ImGui::SetScrollHereY(0.5f); // 画面中央にスクロール
                    lastInsertIdx = insertAfterIdx;
                }
            }
        }

        // ループが終わっても終了境界線が描画されていなければ、最後に表示する
        if (isPaused && !drawnEndBoundary && latestReplayTime >= 0.0f) {
            std::string boundaryStr = "--> ------ 【リプレイ終了地点: " + FormatPlainGameTimestamp(latestReplayTime) + "】 ------";
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 1.0f, 1.0f));
            ImGui::TextUnformatted(boundaryStr.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Double-click to seek Replay to end time\n[左ダブルクリックでリプレイ終了時間にジャンプ]");
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    ReplaySystem::GetInstance()->SeekToTimestamp(latestReplayTime);
                }
            }
            ImGui::PopStyleColor();
            drawnEndBoundary = true;
        }

        // 新規ログ追加時に自動で最下部へスクロール
        // 通常動作中（maxTimestamp < 0.0f）か、ポーズ中であってもシークProgressがほぼ最新（1.0f）かつフィルターOFFの場合
        bool isAtLatestSeek = (ReplaySystem::GetInstance()->GetSeekPos() >= 0.99f);
        bool shouldScroll = (maxTimestamp < 0.0f) || (isPaused && isAtLatestSeek && !filterReplayRangeOnly);
        if (shouldScroll && !isOverlapped && logMessages_.size() > lastLogSize_) {
            ImGui::SetScrollHereY(1.0f);
            lastLogSize_ = logMessages_.size();
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

void Log::Update(float deltaTime) {
    if (!isInitialized_) {
        Initialize();
    }
    activeElapsedTime_ += deltaTime;
}


float Log::GetElapsedTime() {
    if (!isInitialized_) return 0.0f;
    return activeElapsedTime_;
}

bool* Log::GetShowConsolePtr() {
    return &showConsole_;
}

void Log::LogSystemInfo() {
    Log::Write(" ========================================= [システム環境情報] =========================================");

    // 1. OS情報の取得 (RtlGetVersion を動的リンク)
    std::wstring osInfoStr = L"Unknown Windows Version";
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        typedef LONG(WINAPI* fnRtlGetVersion)(PRTL_OSVERSIONINFOW);
        fnRtlGetVersion pRtlGetVersion = reinterpret_cast<fnRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
        if (pRtlGetVersion) {
            RTL_OSVERSIONINFOW osvi = {};
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (pRtlGetVersion(&osvi) == 0) { // STATUS_SUCCESS
                osInfoStr = std::format(L"Windows {}.{} (Build {})", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            }
        }
    }
    Log::Write(std::format(L" ├─ 【OSバージョン】 {}", osInfoStr));

    // 2. CPU情報の取得 (__cpuid)
    constexpr size_t kCpuBrandLength = 49;
    char cpuBrand[kCpuBrandLength] = { 0 };
    int cpuInfo[4] = { 0 };
    
    // CPUIDの各サブリーフ値（CPUID関数）
    constexpr int kCpuIdBrandPart1 = 0x80000002;
    constexpr int kCpuIdBrandPart2 = 0x80000003;
    constexpr int kCpuIdBrandPart3 = 0x80000004;
    constexpr size_t kInfoChunkSize = sizeof(cpuInfo);

    __cpuid(cpuInfo, kCpuIdBrandPart1);
    std::memcpy(cpuBrand, cpuInfo, kInfoChunkSize);
    __cpuid(cpuInfo, kCpuIdBrandPart2);
    std::memcpy(cpuBrand + 16, cpuInfo, kInfoChunkSize);
    __cpuid(cpuInfo, kCpuIdBrandPart3);
    std::memcpy(cpuBrand + 32, cpuInfo, kInfoChunkSize);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    std::string cpuStr(cpuBrand);
    
    // 前後の空白をトリム
    constexpr char kWhitespaceChars[] = " ";
    cpuStr.erase(0, cpuStr.find_first_not_of(kWhitespaceChars));
    cpuStr.erase(cpuStr.find_last_not_of(kWhitespaceChars) + 1);

    Log::Write(std::format(L" ├─ 【CPU】 {} ({} threads)", ConvertString(cpuStr), sysInfo.dwNumberOfProcessors));

    // 3. メモリ容量の取得
    MEMORYSTATUSEX memStatus = {};
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        constexpr float kBytesToGB = 1024.0f * 1024.0f * 1024.0f;
        float totalPhysMemGB = static_cast<float>(memStatus.ullTotalPhys) / kBytesToGB;
        float availPhysMemGB = static_cast<float>(memStatus.ullAvailPhys) / kBytesToGB;
        Log::Write(std::format(L" ├─ 【システムメモリ(RAM)】 総容量: {:.2f} GB | 空き容量: {:.2f} GB", totalPhysMemGB, availPhysMemGB));
    }

    // 4. 実行環境とビルド情報
    std::string buildConfig = "Release";
#ifdef _DEBUG
    buildConfig = "Debug";
#endif
    Log::Write(std::format(L" ├─ 【ビルド構成】 {}", ConvertString(buildConfig)));

    std::wstring currentPath = std::filesystem::current_path().wstring();
    Log::Write(std::format(L" ├─ 【作業ディレクトリ】 {}", currentPath));

    Log::Write(" ======================================================================================================");
}

