#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/StringUtility.h"

// 静的メンバ変数の実体定義
std::ofstream Log::logStream_;
std::string Log::logFileName_;
bool Log::isInitialized_ = false;
std::chrono::steady_clock::time_point Log::startTime_;

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

