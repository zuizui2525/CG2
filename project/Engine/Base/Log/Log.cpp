#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/StringUtility.h"
#include "imgui.h"

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
        
        std::string markerStr = std::format("\n[time|{:02d}h:{:02d}m:00.000s] -------------------- 【 {}分経過 】 --------------------\n", h, m, totalMin);
        std::wstring wMarkerStr = ConvertString(markerStr);

        if (logStream_.is_open()) {
            logStream_ << markerStr << std::endl;
        }
        OutputDebugStringW(wMarkerStr.c_str());
    }

    // 指定フォーマットで時間文字列を作成
    std::string timeStr = std::format("[time|{:02d}h:{:02d}m:{:02d}.{:03d}s] ", h, m, s, ms);
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
        ImGui::Separator();

        // スクロール可能な子ウィンドウ
        ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        // タイムスタンプ文字列フォーマット用ラムダ式
        auto FormatTimestamp = [](float seconds) -> std::string {
            long long elapsedMs = static_cast<long long>(seconds * 1000.0f);
            long long ms = elapsedMs % 1000;
            long long totalSec = elapsedMs / 1000;
            long long s = totalSec % 60;
            long long m = (totalSec / 60) % 60;
            long long h = totalSec / 3600;
            return std::format("[time|{:02d}h:{:02d}m:{:02d}.{:03d}s] ", h, m, s, ms);
        };

        // 最新のリプレイレコードのタイムスタンプを取得
        float latestReplayTime = ReplaySystem::GetInstance()->GetLatestRecordTimestamp();
        constexpr float kMaxReplayTimeRange = 60.0f;

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

        // 被り判定の閾値 (0.15秒 ＝ 150ミリ秒)
        constexpr float kOverlapThreshold = 0.15f;
        bool isOverlapped = (nearestIdx != static_cast<size_t>(-1) && minDiff <= kOverlapThreshold);

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
        if (!isOverlapped && insertAfterIdx == static_cast<size_t>(-2)) {
            std::string timerStr = "--> " + FormatTimestamp(currentTime) + "------ 【進行中】 ------";
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            ImGui::TextUnformatted(timerStr.c_str());
            ImGui::PopStyleColor();
        }

        for (size_t i = 0; i < logMessages_.size(); ++i) {
            const auto& log = logMessages_[i];
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルト白
            
            bool isCurrentActive = (isOverlapped && i == nearestIdx);
            bool isTooOld = (latestReplayTime >= 0.0f && (latestReplayTime - logTimestamps_[i] > kMaxReplayTimeRange));

            if (isCurrentActive) {
                color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // 被ったログは鮮やかな赤で表示！
            } else if (isTooOld) {
                color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // 60秒以上前の古いログは黄色で表示！
            } else {
                // ログの種類に応じた通常の色分け
                if (log.find("error") != std::string::npos || log.find("failed") != std::string::npos || log.find("Error") != std::string::npos) {
                    color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // エラー：マイルドレッド
                } else if (log.find("warning") != std::string::npos || log.find("Warning") != std::string::npos) {
                    color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // 警告：マイルドイエロー
                } else if (log.find("【") != std::string::npos || log.find("起動") != std::string::npos || log.find("完了") != std::string::npos) {
                    color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // システム：マイルドグリーン
                }
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
                if (isTooOld) {
                    ImGui::SetTooltip("Too old to seek Replay (Over 60s ago)\n[60秒以上前のログのためリプレイジャンプできません]");
                } else {
                    ImGui::SetTooltip("Double-click to seek Replay to this log's time\n[左ダブルクリックでリプレイをこのログの時間にジャンプ]");
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        ReplaySystem::GetInstance()->SeekToTimestamp(logTimestamps_[i]);
                    }
                }
            }

            ImGui::PopStyleColor();

            // ログの行間に仮想タイマーを挿入する場合
            if (!isOverlapped && insertAfterIdx == i) {
                std::string timerStr = "--> " + FormatTimestamp(currentTime) + "------ 【進行中】 ------";
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

        // 新規ログ追加時に自動で最下部へスクロール（リプレイ中・非被り中でない場合のみ）
        if (maxTimestamp < 0.0f && !isOverlapped && logMessages_.size() > lastLogSize_) {
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

