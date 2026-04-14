#include "Log.h"

Log::Log() {
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
        OutputDebugStringA("Log file could not be created.\n");
    } else {
        OutputDebugStringA(("Log file created: " + logFileName_ + "\n").c_str());
    }
}

Log::~Log() {
    if (logStream_.is_open()) {
        logStream_.close();
    }
}

void Log::Write(const std::string& message) {
    if (logStream_.is_open()) {
        logStream_ << message << std::endl;
    }
    OutputDebugStringA((message + "\n").c_str());
}

void Log::Write(std::ostream& os, const std::string& message) {
    os << message << std::endl;
    Write(message); // ファイル＆デバッグ出力にも流す
}
