#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/StringUtility.h"

#include "imgui.h"

// 静的メンバ変数の実体定義
std::ofstream Log::logStream_;
std::string Log::logFileName_;
bool Log::isInitialized_ = false;
std::chrono::steady_clock::time_point Log::startTime_;

std::vector<std::string> Log::logMessages_;
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

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - startTime_;

    // ミリ秒、秒、分、時間を計算
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    long long ms = elapsedMs % 1000;
    long long totalSec = elapsedMs / 1000;
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
    }
    logMessages_.push_back(fullMessage);

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
}

void Log::DrawConsoleWindow() {
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
        
        for (const auto& log : logMessages_) {
            // ログの種類に応じた色分け
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルト白
            
            if (log.find("error") != std::string::npos || log.find("failed") != std::string::npos || log.find("Error") != std::string::npos) {
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // エラー：マイルドレッド
            } else if (log.find("warning") != std::string::npos || log.find("Warning") != std::string::npos) {
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // 警告：マイルドイエロー
            } else if (log.find("【") != std::string::npos || log.find("起動") != std::string::npos || log.find("完了") != std::string::npos) {
                color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // システム：マイルドグリーン
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.c_str());
            ImGui::PopStyleColor();
        }

        // 新規ログ追加時に自動で最下部へスクロール
        if (logMessages_.size() > lastLogSize_) {
            ImGui::SetScrollHereY(1.0f);
            lastLogSize_ = logMessages_.size();
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

